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

    void append_to_group();

    void remove_from_group();

    void recover();

    void update_a_doc();

    void get_multi_price_history();

private:
    bool check_product_manager_();
    bool require_docid_list_();
    bool require_str_docid_list_();
    bool require_uuid_();
    bool require_doc_();
    bool require_date_range_();

private:
    boost::shared_ptr<ProductManager> product_manager_;
    std::vector<uint32_t> docid_list_;
    std::vector<std::string> str_docid_list_;
    izenelib::util::UString uuid_;
    izenelib::util::UString from_date_;
    izenelib::util::UString to_date_;
    time_t from_tt_;
    time_t to_tt_;
    Document doc_;
};

/// @}

} // namespace sf1r

#endif // PROCESS_CONTROLLERS_PRODUCT_CONTROLLER_H
