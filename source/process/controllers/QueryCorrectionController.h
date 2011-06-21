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
class  QueryLogSearchService;
class QueryCorrectionController : public Sf1Controller
{
public:
    QueryCorrectionController();

    QueryCorrectionController(const QueryCorrectionController& controller);

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

#endif // PROCESS_CONTROLLERS_QUERY_CORRECTION_CONTROLLER_H
