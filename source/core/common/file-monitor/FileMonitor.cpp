#include "FileMonitor.h"
#include <sys/inotify.h>
#include <unistd.h>
#include <stdexcept>
#include <glog/logging.h>

using namespace sf1r;

namespace
{
const int kEventSize = sizeof(struct inotify_event);
const int kBufferSize = 1024 * (kEventSize + 16);
}

FileMonitor::FileMonitor()
    : isClose_(false)
{
    fileId_ = inotify_init();

    if (fileId_ < 0)
    {
        throw std::runtime_error("error in inotify_init()");
    }
}

FileMonitor::~FileMonitor()
{
    close_();
}

void FileMonitor::close_()
{
    if (isClose_)
        return;

    for (std::vector<int>::const_iterator it = watchIds_.begin();
         it != watchIds_.end(); ++it)
    {
        inotify_rm_watch(fileId_, *it);
    }

    workThread_.interrupt();
    workThread_.join();

    close(fileId_);
    isClose_ = true;
}

bool FileMonitor::addWatch(const std::string& path, uint32_t mask)
{
    int wd = inotify_add_watch(fileId_, path.c_str(), mask);

    if (wd < 0)
    {
        LOG(WARNING) << "failed in watching path " << path;
        return false;
    }

    watchIds_.push_back(wd);
    return true;
}

void FileMonitor::monitor()
{
    if (watchIds_.empty())
    {
        LOG(WARNING) << "no file is watched";
        return;
    }

    workThread_ = boost::thread(&FileMonitor::monitorLoop_, this);
}

void FileMonitor::monitorLoop_()
{
    char buffer[kBufferSize];

    while (true)
    {
        int length = read(fileId_, buffer, kBufferSize); // block until data arrive
        LOG(INFO) << "length: " << length;

        if (length < 0)
        {
            LOG(ERROR) << "error in read(), fileId_: " << fileId_;
            continue;
        }

        int i = 0;
        while (i < length)
        {
            struct inotify_event *event = (struct inotify_event*)&buffer[i];

            if (IN_IGNORED & event->mask)
            {
                LOG(INFO) << "received inotify event IN_IGNORED, exit work thread";
                return;
            }

            LOG(INFO) << "received inotify event mask: " << event->mask
                      << ", file name: " << event->name;

            LOG(INFO) << "i: " << i
                      << ", event->len: " << event->len;

            if (process(event->name, event->mask))
            {
                LOG(INFO) << "finished inotify event process";
            }
            else
            {
                LOG(ERROR) << "failed inotify event process";
            }

            i += kEventSize + event->len;
        }
    }
}
