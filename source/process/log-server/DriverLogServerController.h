#ifndef DRIVER_LOG_SERVER_CONTROLLER_H_
#define DRIVER_LOG_SERVER_CONTROLLER_H_

#include <iostream>

#include <util/driver/Controller.h>

#include <common/Keys.h>

namespace sf1r
{

class DriverLogServerController : public izenelib::driver::Controller
{
public:
    // TODO
    void update_uuid();
};

}

#endif /* DRIVER_LOG_SERVER_CONTROLLER_H_ */
