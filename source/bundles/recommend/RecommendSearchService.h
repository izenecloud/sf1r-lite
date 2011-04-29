/**
 * @file RecommendSearchService.h
 * @author Jun Jiang
 * @date 2011-04-20
 */

#ifndef RECOMMEND_SEARCH_SERVICE_H
#define RECOMMEND_SEARCH_SERVICE_H

#include <recommend-manager/RecTypes.h>

#include <util/osgi/IService.h>

#include <string>
#include <vector>

namespace sf1r
{
class User;
class Item;
class UserManager;
class ItemManager;
class RecommendManager;

class RecommendSearchService : public ::izenelib::osgi::IService
{
public:
    RecommendSearchService(
        UserManager* userManager,
        ItemManager* itemManager,
        RecommendManager* recommendManager,
        RecIdGenerator* userIdGenerator,
        RecIdGenerator* itemIdGenerator
    );

    bool getUser(const std::string& userIdStr, User& user);

    bool getItem(const std::string& itemIdStr, Item& item);

    bool recommend(
        RecommendType recType,
        int maxRecNum,
        const std::string& userIdStr,
        const std::vector<std::string>& inputItemVec,
        const std::vector<std::string>& includeItemVec,
        const std::vector<std::string>& excludeItemVec,
        std::vector<Item>& recItemVec,
        std::vector<double> recWeightVec
    );

private:
    bool convertItemId(
        const std::vector<std::string>& inputItemVec,
        std::vector<itemid_t>& outputItemVec
    );

private:
    UserManager* userManager_;
    ItemManager* itemManager_;
    RecommendManager* recommendManager_;
    RecIdGenerator* userIdGenerator_;
    RecIdGenerator* itemIdGenerator_;
};

} // namespace sf1r

#endif // RECOMMEND_SEARCH_SERVICE_H
