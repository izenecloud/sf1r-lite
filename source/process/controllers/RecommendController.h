#ifndef PROCESS_CONTROLLERS_RECOMMEND_CONTROLLER_H
#define PROCESS_CONTROLLERS_RECOMMEND_CONTROLLER_H
/**
 * @file process/controllers/RecommendController.h
 * @author Jun Jiang
 * @date Created <2011-04-21>
 */
#include "Sf1Controller.h"

#include <string>

namespace sf1r
{
class User;
class Item;

/// @addtogroup controllers
/// @{

/**
 * @brief Controller \b recommend
 *
 * Add, update, remove or fetch users and items,
 * add purchase and visit events,
 * and do recommendation.
 */
class RecommendController : public Sf1Controller
{
public:
    void add_user();
    void update_user();
    void remove_user();
    void get_user();

    void add_item();
    void update_item();
    void remove_item();
    void get_item();

    void visit_item();
    void purchase_item();

    void do_recommend();

private:
    bool requireProperty(const std::string& propName);
    bool value2User(const izenelib::driver::Value& value, User& user);
    bool value2Item(const izenelib::driver::Value& value, Item& item);
    bool value2ItemIdVec(const std::string& propName, std::vector<std::string>& itemIdVec);
};

/// @}

} // namespace sf1r

#endif // PROCESS_CONTROLLERS_RECOMMEND_CONTROLLER_H
