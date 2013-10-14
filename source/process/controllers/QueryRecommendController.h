#ifndef PROCESS_CONTROLLERS_QUERY_RECOMMEND_CONTROLLER_H
#define PROCESS_CONTROLLERS_QUERY_RECOMMEND_CONTROLLER_H

#include "Sf1Controller.h"

namespace sf1r
{

/// @addtogroup controllers
/// @{

/**
 * @brief Controller \b query_recommend 
 *
 */
class QueryRecommendController : public Sf1Controller
{
public:
    QueryRecommendController();

    void recommend();

protected:
    virtual bool checkCollectionService(std::string& error);
};

/// @}

} // namespace sf1r

#endif
