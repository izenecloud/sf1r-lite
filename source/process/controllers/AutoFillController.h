#ifndef PROCESS_CONTROLLERS_AUTO_FILL_CONTROLLER_H
#define PROCESS_CONTROLLERS_AUTO_FILL_CONTROLLER_H
/**
 * @file process/controllers/AutoFillController.h
 * @author Ian Yang
 * @date Created <2010-05-31 14:11:35>
 */
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
class QueryLogSearchService; 
class AutoFillController : public Sf1Controller
{
public:
    AutoFillController();

    AutoFillController(const AutoFillController& controller);

    enum
    {
        kDefaultCount = 8       /**< Default count of result */
    };
    enum
    {
        kMaxCount = 100         /**< Max count of result */
    };
    void setService(QueryLogSearchService* queryLogSearchService)
    {
        queryLogSearchService_ = queryLogSearchService;
    }

    void index();
private:
    QueryLogSearchService* queryLogSearchService_;
};

/// @}

} // namespace sf1r

#endif // PROCESS_CONTROLLERS_AUTO_FILL_CONTROLLER_H
