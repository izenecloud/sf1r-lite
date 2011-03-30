#ifndef PROCESS_CONTROLLERS_TEST_CONTROLLER_H
#define PROCESS_CONTROLLERS_TEST_CONTROLLER_H
/**
 * @file process/controllers/TestController.h
 * @author Ian Yang
 * @date Created <2010-07-02 14:48:13>
 */

#include "Sf1Controller.h"

namespace sf1r
{

/// @addtogroup controllers
/// @{

/**
 * @brief Controller \b test
 *
 * Various features for test
 */
class TestController : public Sf1Controller
{
public:
    void echo();

    void sleep();
};

/// @}

} // namespace sf1r

#endif // PROCESS_CONTROLLERS_TEST_CONTROLLER_H
