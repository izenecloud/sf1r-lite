#ifndef DRIVER_LOG_SERVER_CONTROLLER_H_
#define DRIVER_LOG_SERVER_CONTROLLER_H_

#include <iostream>

#include <util/driver/Controller.h>

#include <common/Keys.h>

namespace sf1r
{

/**
 * @brief Controller \b log_server
 * Log server services
 */
class DriverLogServerController : public izenelib::driver::Controller
{
public:
    void update_cclog();
};

}

#endif /* DRIVER_LOG_SERVER_CONTROLLER_H_ */
