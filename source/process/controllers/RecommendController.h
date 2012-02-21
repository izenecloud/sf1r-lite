#ifndef PROCESS_CONTROLLERS_RECOMMEND_CONTROLLER_H
#define PROCESS_CONTROLLERS_RECOMMEND_CONTROLLER_H
/**
 * @file process/controllers/RecommendController.h
 * @author Jun Jiang
 * @date Created <2011-04-21>
 */
#include "Sf1Controller.h"
#include <util/osgi/IService.h>

#include <string>

namespace sf1r
{
class User;
class Item;
class ItemCondition;
struct RecommendParam;
struct RecommendItem;
struct TIBParam;
struct ItemBundle;
struct RateParam;

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

    void visit_item();
    void purchase_item();
    void update_shopping_cart();
    void track_event();
    void rate_item();

    void do_recommend();

    void top_item_bundle();

private:
    bool requireService(::izenelib::osgi::IService* service);
    bool requireProperty(
        const std::string& propName,
        std::string& propValue
    );
    bool value2User(User& user);
    bool value2ItemIdVec(const std::string& propName, std::vector<std::string>& itemIdVec);
    bool value2ItemCondition(ItemCondition& itemCondition);
    bool value2SelectProps(std::vector<std::string>& propNames);

    bool parseRecommendParam(RecommendParam& param);
    void renderRecommendResult(const RecommendParam& param, const std::vector<RecommendItem>& recItemVec);

    bool parseTIBParam(TIBParam& param);
    void renderBundleResult(const TIBParam& param, const std::vector<ItemBundle>& bundleVec);

    bool parseRateParam(RateParam& param);
};

/// @}

} // namespace sf1r

#endif // PROCESS_CONTROLLERS_RECOMMEND_CONTROLLER_H
