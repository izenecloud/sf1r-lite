#ifndef DRIVER_LOG_SERVER_CONTROLLER_H_
#define DRIVER_LOG_SERVER_CONTROLLER_H_

#include <util/driver/Controller.h>

#include <iostream>
#include <set>

using namespace izenelib::driver;

namespace sf1r
{

/**
 * @brief Controller \b log_server
 * Log server for updating cclog
 */
class LogDispatchHandler;
class DriverLogServerController : public izenelib::driver::Controller
{
public:
    DriverLogServerController(
        const boost::shared_ptr<LogDispatchHandler>& driverLogServerHandler)
    {
        log_dispatch_handler_ = driverLogServerHandler;
        BOOST_ASSERT(log_dispatch_handler_);
    }

    /// overwrite
    bool preprocess();

    void update_cclog();

    void backup_raw_cclog();

    void convert_raw_cclog();

    void update_scd(); // reserved

    void update_documents();

    void flush();

private:
    boost::shared_ptr<LogDispatchHandler> log_dispatch_handler_;
};

}

#endif /* DRIVER_LOG_SERVER_CONTROLLER_H_ */
