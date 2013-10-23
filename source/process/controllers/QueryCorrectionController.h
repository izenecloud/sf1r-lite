#ifndef PROCESS_CONTROLLERS_QUERY_CORRECTION_CONTROLLER_H
#define PROCESS_CONTROLLERS_QUERY_CORRECTION_CONTROLLER_H

#include "Sf1Controller.h"

namespace sf1r
{
class MiningSearchService;

/// @addtogroup controllers
/// @{

/**
 * @brief Controller \b query_correction
 *
 * Gets correct query for origianl query.
 */
class QueryCorrectionController : public Sf1Controller
{
public:
    QueryCorrectionController();

    void index();
    void evaluate();

protected:
    virtual bool checkCollectionService(std::string& error);

private:
    MiningSearchService* miningSearchService_;
};

/// @}

} // namespace sf1r

#endif // PROCESS_CONTROLLERS_QUERY_CORRECTION_CONTROLLER_H
