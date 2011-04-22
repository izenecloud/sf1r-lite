/**
 * @file RecommendSearchService.h
 * @author Jun Jiang
 * @date 2011-04-20
 */

#ifndef RECOMMEND_SEARCH_SERVICE_H
#define RECOMMEND_SEARCH_SERVICE_H

#include <util/osgi/IService.h>

#include <string>

namespace sf1r
{
class User;
class UserManager;

class RecommendSearchService : public ::izenelib::osgi::IService
{
public:
    RecommendSearchService(UserManager* userManager);

    bool getUser(const std::string& userIdStr, User& user);

private:
    UserManager* userManager_;
};

} // namespace sf1r

#endif // RECOMMEND_SEARCH_SERVICE_H
