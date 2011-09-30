#ifndef PROCESS_CONTROLLERS_AUTO_FILL_CONTROLLER_H
#define PROCESS_CONTROLLERS_AUTO_FILL_CONTROLLER_H
/**
 * @file process/controllers/AutoFillController.h
 * @author Ian Yang
 * @date Created <2010-05-31 14:11:35>
 */
#include "CollectionMiningController.h"

namespace sf1r
{
class RecommendManager;
/// @addtogroup controllers
/// @{

/**
 * @brief Controller \b auto_fill
 *
 * Gets list of popular keywords starting with specified prefix.
 */
class QueryLogSearchService; 
class AutoFillController : public CollectionMiningController
{
public:

    enum
    {
        kDefaultCount = 8       /**< Default count of result */
    };
    enum
    {
        kMaxCount = 100         /**< Max count of result */
    };
    void index();
    
private:
    bool check_recommend_manager_();
    
private:
    boost::shared_ptr<RecommendManager> recommend_manager_;
};

/// @}

} // namespace sf1r

#endif // PROCESS_CONTROLLERS_AUTO_FILL_CONTROLLER_H
