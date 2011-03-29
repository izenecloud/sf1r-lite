#ifndef BA_PROCESS_CONTROLLERS_QUERY_CORRECTION_CONTROLLER_H
#define BA_PROCESS_CONTROLLERS_QUERY_CORRECTION_CONTROLLER_H


#include "Sf1Controller.h"
#include <mining-manager/query-correction-submanager/QueryCorrectionSubmanager.h>
namespace sf1r {

/// @addtogroup controllers
/// @{

/**
 * @brief Controller \b auto_fill
 *
 * Gets list of popular keywords starting with specified prefix.
 */
class QueryCorrectionController : public Sf1Controller
{
public:
    void index();
};

/// @}

} // namespace sf1r

#endif // BA_PROCESS_CONTROLLERS_QUERY_CORRECTION_CONTROLLER_H
