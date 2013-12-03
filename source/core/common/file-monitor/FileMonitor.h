/**
 * @file FileMonitor.h
 * @brief It supports monitor file change, and trigger the @c FileEventHandler
 *        when event happens.
 */

#ifndef SF1R_FILE_MONITOR_H
#define SF1R_FILE_MONITOR_H

#include "FileEventHandler.h"
#include <vector>
#include <string>
#include <boost/thread/thread.hpp>

namespace sf1r
{

class FileMonitor
{
public:
    FileMonitor();
    ~FileMonitor();

    void setFileEventHandler(FileEventHandler* handler);

    bool addWatch(const std::string& path, uint32_t mask);

    void monitor();

private:
    void monitorLoop_();

private:
    int fileId_;
    std::vector<int> watchIds_;
    boost::thread workThread_;
    FileEventHandler* handler_;
};

} // namespace sf1r

#endif // SF1R_FILE_MONITOR_H
