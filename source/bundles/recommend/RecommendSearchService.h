/**
 * @file RecommendSearchService.h
 * @author Jun Jiang
 * @date 2011-04-20
 */

#ifndef RECOMMEND_SEARCH_SERVICE_H
#define RECOMMEND_SEARCH_SERVICE_H

#include <recommend-manager/RecTypes.h>
#include <recommend-manager/Item.h>
#include <recommend-manager/RecommendItem.h>

#include <util/osgi/IService.h>

#include <string>
#include <vector>

namespace sf1r
{
class User;
class UserManager;
class ItemManager;
class RecommenderManager;
struct RecommendParam;

class RecommendSearchService : public ::izenelib::osgi::IService
{
public:
    RecommendSearchService(
        UserManager* userManager,
        ItemManager* itemManager,
        RecommenderManager* recommenderManager,
        RecIdGenerator* userIdGenerator,
        RecIdGenerator* itemIdGenerator
    );

    bool getUser(const std::string& userIdStr, User& user);

    bool getItem(const std::string& itemIdStr, Item& item);

    bool recommend(
        RecommendParam& param,
        std::vector<RecommendItem>& recItemVec
    );

    bool topItemBundle(
        int maxRecNum,
        int minFreq,
        std::vector<vector<Item> >& bundleVec,
        std::vector<int>& freqVec
    );

private:
    bool convertIds_(RecommendParam& param);
    bool convertItemId_(
        const std::vector<std::string>& inputItemVec,
        std::vector<itemid_t>& outputItemVec
    );
    bool getItems_(std::vector<RecommendItem>& recItemVec) const;

private:
    UserManager* userManager_;
    ItemManager* itemManager_;
    RecommenderManager* recommenderManager_;
    RecIdGenerator* userIdGenerator_;
    RecIdGenerator* itemIdGenerator_;
};

} // namespace sf1r

#endif // RECOMMEND_SEARCH_SERVICE_H
