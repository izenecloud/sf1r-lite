/**
 * @file FileMonitor.h
 * @brief This class is used as a parent class fo file monitor.
 *        It supports monitor/process when event happens.
 */

#ifndef SF1R_FILE_MONITOR_H
#define SF1R_FILE_MONITOR_H

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/inotify.h>

#include <vector>
#include <string>
#include <glog/logging.h>
#include <boost/thread/thread.hpp> 

namespace sf1r
{
class FileMonitor
{
enum {
    EVENT_SIZE = sizeof(struct inotify_event),
    BUF_LEN = 1024 * (EVENT_SIZE + 16)
};

public:
    FileMonitor()
    {
        fd_ = inotify_init();
    }

    ~FileMonitor()
    {
        for (std::vector<int>::iterator i = wd_list_.begin(); i != wd_list_.end(); ++i)
            inotify_rm_watch(fd_, *i);
        close(fd_);
    }

    void addWatch(const std::string& path, uint32_t mask)
    {
        wd_list_.push_back(inotify_add_watch(fd_, path.c_str(), mask));
    }

    void monitor()
    {
         thread_ = boost::thread(&FileMonitor::do_monitor_, this);
    }

    virtual bool process(const std::string& fileName, uint32_t mask) = 0;

private:
    void do_monitor_()
    {
        char buffer[BUF_LEN];
        while (true)
        {
            int length, i = 0;
            length = read( fd_, buffer, BUF_LEN ); //suspend

            if ( length < 0 ) 
            {
                LOG(ERROR) << "read fd_: " << fd_;
            }
            while (i < length)
            {
                struct inotify_event *event = (struct inotify_event*)&buffer[i];

                if(process(event->name, event->mask))
                {
                    LOG(INFO) << "one success resource update ...";
                }
                else
                {
                    LOG(ERROR) << "update resource wrong ... ";
                }
                
                i += EVENT_SIZE + event->len;
            }
        }
    }

    int fd_;
    std::vector<int> wd_list_;
    boost::thread thread_;
};
}


#endif ///SF1R_FILE_MONITOR_H