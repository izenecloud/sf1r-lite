#ifndef RESOURCE_MONITOR_H
#define RESOURCE_MONITOR_H

#include <boost/function.hpp>
#include <boost/thread.hpp>

namespace sf1r
{
typedef boost::function<void(const std::string&, uint32_t)> ResourceHandler;
class ResourceMonitor
{
public:
    ResourceMonitor(const std::string& dir, uint32_t mask);
    ~ResourceMonitor();

public:
    void setResourceHandler(ResourceHandler* handler)
    {
        handler_ = handler;
    }

    void monitor();

private:
    bool addWatch_(const std::string& path, uint32_t mask);
    void monitorLoop_();
    void waitFinish_(const std::string& dir);

private:
    int fd_;
    std::string dir_;
    uint32_t mask_;
    std::vector<int> watchIds_;
    ResourceHandler* handler_;
    boost::thread workThread_;
};
}

#endif
