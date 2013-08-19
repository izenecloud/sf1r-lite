#include "KNlpDictMonitor.h"
#include <common/ResourceManager.h>
#include <util/singleton.h>
#include <sys/inotify.h>

using namespace sf1r;

namespace
{
const std::string kMonitorFileName = "z.update.time";
const uint32_t kMonitorEvent = IN_ATTRIB;
}

KNlpDictMonitor* KNlpDictMonitor::get()
{
    return izenelib::util::Singleton<KNlpDictMonitor>::get();
}

KNlpDictMonitor::KNlpDictMonitor()
    : isStart_(false)
{
}

bool KNlpDictMonitor::handle(const std::string& fileName, uint32_t mask)
{
    if (mask & kMonitorEvent)
    {
        boost::shared_ptr<KNlpWrapper> knlpWrapper(new KNlpWrapper(dictDir_));
        if (!knlpWrapper->loadDictFiles())
            return false;

        KNlpResourceManager::setResource(knlpWrapper);
    }

    return true;
}

void KNlpDictMonitor::start(const std::string& dictDir)
{
    if (isStart_)
        return;

    dictDir_ = dictDir;
    std::string fileName = dictDir_ + "/" + kMonitorFileName;

    if (!monitor_.addWatch(fileName, kMonitorEvent))
        return;

    monitor_.setFileEventHandler(this);
    monitor_.monitor();
    isStart_ = true;
}
