#ifndef PROCESS_CONTROLLERS_QUERY_CORRECTION_CONTROLLER_H
#define PROCESS_CONTROLLERS_QUERY_CORRECTION_CONTROLLER_H

#include "Sf1Controller.h"

namespace sf1r
{

/// @addtogroup controllers
/// @{

/**
 * @brief Controller \b auto_fill
 *
 * Gets list of popular keywords starting with specified prefix.
 */
class QueryCorrectionController : public ::izenelib::driver::Controller
{
public:
    enum
    {
        kDefaultCount = 8       /**< Default count of result */
    };

    void index();
};

/// @}

} // namespace sf1r

#endif // PROCESS_CONTROLLERS_QUERY_CORRECTION_CONTROLLER_H
