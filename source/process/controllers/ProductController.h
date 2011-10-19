#ifndef PROCESS_CONTROLLERS_PRODUCT_CONTROLLER_H
#define PROCESS_CONTROLLERS_PRODUCT_CONTROLLER_H
/**
 * @file process/controllers/ProductController.h
 * @author Jia Guo
 */
#include "Sf1Controller.h"
#include "CollectionHandler.h"
namespace sf1r
{

class ProductManager;
/// @addtogroup controllers
/// @{

/**
 * @brief Controller \b product.
 *
 * Provides price comparasion and automatic categorization functions.
 */
class ProductController : public Sf1Controller
{
public:

    void add_new_group();
    
//     void add_docs_to_tid();
//     
//     void remove_docs_from_tid();
//     
//     void get_all_product_info();

private:
    bool check_product_manager_();
    bool require_docs_();
    
private:
    boost::shared_ptr<ProductManager> product_manager_;
    std::vector<uint32_t> docid_list_;
};

/// @}

} // namespace sf1r

#endif // PROCESS_CONTROLLERS_PRODUCT_CONTROLLER_H


