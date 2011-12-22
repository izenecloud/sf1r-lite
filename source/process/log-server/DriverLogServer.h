#ifndef DRIVER_LOG_SERVER_H_
#define DRIVER_LOG_SERVER_H_

#include <util/driver/DriverServer.h>

#include <boost/thread.hpp>

namespace sf1r
{

class DriverLogServer
{
public:
    DriverLogServer(uint16_t port, uint32_t threadNum);

    ~DriverLogServer()
    {
    }

    uint16_t getPort() const
    {
        return port_;
    }

    bool init();

    void start();

    void join();

    void stop();

private:
    bool initRouter();

private:
    uint16_t port_;
    uint32_t threadNum_;

    bool bStarted_;
    boost::scoped_ptr<boost::thread> driverThread_;

    boost::scoped_ptr<izenelib::driver::DriverServer> driverServer_;
    boost::shared_ptr<izenelib::driver::Router> router_;
};

}

#endif /* DRIVER_LOG_SERVER_H_ */
