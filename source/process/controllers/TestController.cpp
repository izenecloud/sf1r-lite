/**
 * @file process/controllers/TestController.cpp
 * @author Ian Yang
 * @date Created <2010-07-02 14:49:40>
 */
#include "TestController.h"

#include <common/Keys.h>

#include <unistd.h>

namespace sf1r
{

using driver::Keys;

/**
 * @brief Action \b echo. Sends back the field "message" in request.
 *
 * @section request
 * - @b message (@c Any): any message.
 *
 * @section response
 * - @b message (@c Any): copied from message and echo back.
 */
void TestController::echo()
{
    response()[Keys::message] = request()[Keys::message];
}

/**
 * @brief Action \b sleep. Do nothing but sleep.
 *
 * @section request
 * - @b seconds (@c Uint): Number of seconds to sleep
 *
 * @section response
 *
 * No extra fields.
 */
void TestController::sleep()
{
    ::sleep(asUint(request()[Keys::seconds]));
}

} // namespace sf1r
