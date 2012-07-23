#ifndef PROCESS_CONTROLLERS_PRODUCT_CONTROLLER_H
#define PROCESS_CONTROLLERS_PRODUCT_CONTROLLER_H
/**
 * @file process/controllers/ProductController.h
 * @author Jia Guo
 */
#include "Sf1Controller.h"
#include "CollectionHandler.h"
#include <product-manager/pm_types.h>
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

    void get_top_price_cut_list();

    void migrate_price_history();
    void finish_hook();

protected:
    virtual bool checkCollectionService(std::string& error);

private:
    bool require_docid_list_();
    bool require_str_docid_list_();
    bool require_uuid_();
    bool maybe_option_();
    bool maybe_doc_(bool must);
    bool require_date_range_();

private:
    boost::shared_ptr<ProductManager> product_manager_;
    std::vector<uint32_t> docid_list_;
    Document doc_;
    ProductEditOption option_;
    std::vector<uint128_t> str_docid_list_;
    izenelib::util::UString uuid_;
    izenelib::util::UString from_date_;
    izenelib::util::UString to_date_;
    std::string prop_name_;
    std::string prop_value_;
    std::string new_keyspace_;
    uint32_t days_;
    uint32_t count_;
    uint32_t start_;
    time_t from_tt_;
    time_t to_tt_;
};

/// @}

} // namespace sf1r

#endif // PROCESS_CONTROLLERS_PRODUCT_CONTROLLER_H
