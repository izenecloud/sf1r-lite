#include "Rebuilder.h"
#include <common/ResourceManager.h>
#include <util/singleton.h>
#include <sys/inotify.h>

using namespace sf1r;

namespace
{
const std::string kMonitorFileName = "z.update.time";
const uint32_t kMonitorEvent = IN_MOVED_TO;
}

Rebuilder* Rebuilder::get()
{
    return izenelib::util::Singleton<Rebuilder>::get();
}

Rebuilder::Rebuilder()
    : isStart_(false)
{
}

bool Rebuilder::handle(const std::string& fileName, uint32_t mask)
{
    if (mask & kMonitorEvent)
    {
        if (fileName != kMonitorFileName)
            return true;
        for (std::size_t i = 0; i < handlers_.size(); i++)
        {
            (*(handlers_[i]))();
        }
    }

    return true;
}

void Rebuilder::addRebuildHanlder(Recommend::RebuildHanlder* handler)
{
    if (NULL != handler)
        handlers_.push_back(handler);
}

void Rebuilder::start(const std::string& dictDir)
{
    if (isStart_)
        return;

    dictDir_ = dictDir;

    if (!monitor_.addWatch(dictDir_, kMonitorEvent))
        return;

    monitor_.setFileEventHandler(this);
    monitor_.monitor();
    isStart_ = true;
}
