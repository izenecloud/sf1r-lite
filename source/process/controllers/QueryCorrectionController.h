#ifndef PROCESS_CONTROLLERS_QUERY_CORRECTION_CONTROLLER_H
#define PROCESS_CONTROLLERS_QUERY_CORRECTION_CONTROLLER_H

#include "Sf1Controller.h"

namespace sf1r
{
class MiningSearchService;

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
    QueryCorrectionController();

    void index();

protected:
    virtual bool checkCollectionService(std::string& error);

private:
    MiningSearchService* miningSearchService_;
};

/// @}

} // namespace sf1r

#endif // PROCESS_CONTROLLERS_QUERY_CORRECTION_CONTROLLER_H
