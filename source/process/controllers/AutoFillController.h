#ifndef BA_PROCESS_CONTROLLERS_AUTO_FILL_CONTROLLER_H
#define BA_PROCESS_CONTROLLERS_AUTO_FILL_CONTROLLER_H
/**
 * @file ba-process/controllers/AutoFillController.h
 * @author Ian Yang
 * @date Created <2010-05-31 14:11:35>
 */
#include "Sf1Controller.h"

namespace sf1r {

/// @addtogroup controllers
/// @{

/**
 * @brief Controller \b auto_fill
 *
 * Gets list of popular keywords starting with specified prefix.
 */
class AutoFillController : public Sf1Controller
{
public:
    enum {
        kDefaultCount = 8       /**< Default count of result */
    };
    enum {
        kMaxCount = 100         /**< Max count of result */
    };

    void index();
};

/// @}

} // namespace sf1r

#endif // BA_PROCESS_CONTROLLERS_AUTO_FILL_CONTROLLER_H
