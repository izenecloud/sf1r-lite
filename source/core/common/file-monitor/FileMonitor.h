/**
 * @file FileMonitor.h
 * @brief This class is used as a parent class for file monitor.
 *        It supports monitor file change, and trigger the process
 *        when event happens.
 */

#ifndef SF1R_FILE_MONITOR_H
#define SF1R_FILE_MONITOR_H

#include <sys/types.h>
#include <vector>
#include <string>
#include <boost/thread/thread.hpp>

namespace sf1r
{

class FileMonitor
{
public:
    FileMonitor();
    virtual ~FileMonitor();

    bool addWatch(const std::string& path, uint32_t mask);

    void monitor();

    virtual bool process(const std::string& fileName, uint32_t mask) = 0;

protected:
    /**
     * Remove file watch, and wait for the work thread finish.
     * @attention this function must be called in sub-class destructor,
     *            otherwise, the sub-class @c process() might be called after
     *            sub-class desctructor, which leads to memory access error.
     */
    void close_();

private:
    void monitorLoop_();

private:
    int fileId_;
    bool isClose_;
    std::vector<int> watchIds_;
    boost::thread workThread_;
};

} // namespace sf1r

#endif // SF1R_FILE_MONITOR_H
