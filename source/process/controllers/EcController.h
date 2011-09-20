#ifndef PROCESS_CONTROLLERS_EC_CONTROLLER_H
#define PROCESS_CONTROLLERS_EC_CONTROLLER_H
/**
 * @file process/controllers/EcController.h
 * @author Jia Guo
 */
#include "CollectionMiningController.h"

namespace sf1r
{
    
namespace ec
{
    class EcManager;
}

/// @addtogroup controllers
/// @{

/**
 * @brief Controller \b ec, Electronic commerce.
 *
 * Provides price comparasion and automatic categorization functions.
 */
class EcController : public CollectionMiningController
{
public:

    void get_all_tid();
    
    void get_docs_by_tid();
    
    void get_tids_by_docs();
    
    void add_new_tid();
    
    void add_docs_to_tid();
    
    void remove_docs_from_tid();
    
private:
    bool check_ec_manager_();
    bool require_tid_();
    bool require_docs_();
    
private:
    boost::shared_ptr<ec::EcManager> ec_manager_;
    uint32_t tid_;
    std::vector<uint32_t> docid_list_;
};

/// @}

} // namespace sf1r

#endif // PROCESS_CONTROLLERS_EC_CONTROLLER_H


