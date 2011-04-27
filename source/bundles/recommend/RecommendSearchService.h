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

namespace sf1r
{
class User;
class Item;
class UserManager;
class ItemManager;

class RecommendSearchService : public ::izenelib::osgi::IService
{
public:
    RecommendSearchService(
        UserManager* userManager,
        ItemManager* itemManager,
        RecIdGenerator* userIdGenerator,
        RecIdGenerator* itemIdGenerator
    );

    bool getUser(const std::string& userIdStr, User& user);

    bool getItem(const std::string& itemIdStr, Item& item);

private:
    UserManager* userManager_;
    ItemManager* itemManager_;
    RecIdGenerator* userIdGenerator_;
    RecIdGenerator* itemIdGenerator_;
};

} // namespace sf1r

#endif // RECOMMEND_SEARCH_SERVICE_H
