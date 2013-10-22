#include "ResourceMonitor.h"

#include <boost/filesystem.hpp>
#include <boost/interprocess/sync/file_lock.hpp>
#include <boost/thread/thread.hpp>
#include <sys/inotify.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/file.h>

namespace sf1r
{

namespace
{
const int kEventSize = sizeof(struct inotify_event);
const int kBufferSize = 1024 * (kEventSize + 16);
}

ResourceMonitor::ResourceMonitor(const std::string& dir, uint32_t mask)
    : fd_(0)
    , dir_(dir)
    , mask_(mask)
    , handler_(NULL)
{
    fd_ = inotify_init();
    if (fd_ < 0)
    {
        throw std::runtime_error("error in inotify_init()");
    }
    addWatch_(dir_, mask_);
}

ResourceMonitor::~ResourceMonitor()
{
    for (std::vector<int>::const_iterator it = watchIds_.begin();
         it != watchIds_.end(); ++it)
    {
        inotify_rm_watch(fd_, *it);
    }

    workThread_.interrupt();
    workThread_.join();

    close(fd_);
}

bool ResourceMonitor::addWatch_(const std::string& path, uint32_t mask)
{
    boost::filesystem::directory_iterator end;
    for (boost::filesystem::directory_iterator it(path) ; it != end ; ++it)
    {
        const std::string p = it->path().string();
        if (boost::filesystem::is_regular_file(p))
        {
            int wd = inotify_add_watch(fd_, p.c_str(), mask);
            if (0 > wd)
                return false;
            watchIds_.push_back(wd);
        }
        else if (boost::filesystem::is_directory(p))
            addWatch_(p, mask);
    }
    return true;
}

void ResourceMonitor::monitor()
{
    if (watchIds_.empty())
    {
        return;
    }

    workThread_ = boost::thread(&ResourceMonitor::monitorLoop_, this);
}
    
void ResourceMonitor::waitFinish_(const std::string& dir)
{
    boost::filesystem::directory_iterator end;
    for (boost::filesystem::directory_iterator it(dir) ; it != end ; ++it)
    {
        const std::string p = it->path().string();
        if (boost::filesystem::is_regular_file(p))
        {
            uint32_t fsize = 0;
            while (true)
            {
                const uint32_t size = boost::filesystem::file_size(it->path());
                if (fsize == size)
                {
                    break;
                }
                else
                {
                    fsize = size;
                }
                sleep(1);
            }
        }
        else if (boost::filesystem::is_directory(p))
            waitFinish_(p);
        sleep(1);
    }
}

void ResourceMonitor::monitorLoop_()
{
    char buffer[kBufferSize];

    while (true)
    {
        int length = read(fd_, buffer, kBufferSize); 
     
        if (boost::this_thread::interruption_requested())
        {
            return;
        }

        if (length < 0)
        {
            continue;
        }
        
        int i = 0;
        bool falseAlarm = true;
        while (i < length)
        {
            struct inotify_event *event = (struct inotify_event*)&buffer[i];
            if (IN_IGNORED & event->mask)
            {
                return;
            }
            if (mask_ & event->mask)
            {
                falseAlarm = false;
                break;
            }
            i += kEventSize + event->len;
        }
        if (falseAlarm)
            continue;

        // Wait for all files modify finish
        waitFinish_(dir_);
        waitFinish_(dir_);
        
        if (NULL != handler_)
            (*handler_)(dir_, mask_);
        
        // Clean Events
        int flags = 0;
        if (-1 == (flags = fcntl(fd_, F_GETFL, 0)))
            flags = 0;
        fcntl(fd_, F_SETFL, flags | O_NONBLOCK);
        
        while (true)
        {
            if (0 > read(fd_, buffer, kBufferSize))
                break;
        }
        // reset 
        fcntl(fd_, F_SETFL, flags & (~O_NONBLOCK));
    }
}


}
