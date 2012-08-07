/**
 * @file RecommendSearchService.h
 * @author Jun Jiang
 * @date 2011-04-20
 */

#ifndef RECOMMEND_SEARCH_SERVICE_H
#define RECOMMEND_SEARCH_SERVICE_H

#include <recommend-manager/common/RecTypes.h>
#include <recommend-manager/common/RecommendItem.h>

#include <util/osgi/IService.h>

#include <string>
#include <vector>

namespace sf1r
{
class User;
class UserManager;
class ItemManager;
class RecommenderFactory;
class ItemIdGenerator;
struct RecommendParam;
struct TIBParam;
struct ItemBundle;
class RecommendBundleConfiguration;
class IndexSearchService;
class QueryBuilder;

class RecommendSearchService : public ::izenelib::osgi::IService
{
public:
    RecommendSearchService(
        RecommendBundleConfiguration& bundleConfig,
        UserManager& userManager,
        ItemManager& itemManager,
        RecommenderFactory& recommenderFactory,
        ItemIdGenerator& itemIdGenerator,
        IndexSearchService* indexSearchService
    );

    bool getUser(const std::string& userId, User& user);

    bool recommend(
        RecommendParam& param,
        std::vector<RecommendItem>& recItemVec
    );

    bool topItemBundle(
        const TIBParam& param,
        std::vector<ItemBundle>& bundleVec
    );

    RecommendType getRecommendType(const std::string& typeStr) const;

private:
    bool convertIds_(RecommendParam& param);
    bool convertItemId_(
        const std::vector<std::string>& inputItemVec,
        std::vector<itemid_t>& outputItemVec
    );

    void getReasonItems_(
        const std::vector<std::string>& selectProps,
        std::vector<RecommendItem>& recItemVec
    );

    void getBundleItems_(
        const std::vector<std::string>& selectProps,
        std::vector<ItemBundle>& bundleVec
    );

private:
    RecommendBundleConfiguration& bundleConfig_;
    UserManager& userManager_;
    ItemManager& itemManager_;
    RecommenderFactory& recommenderFactory_;
    ItemIdGenerator& itemIdGenerator_;

    QueryBuilder* queryBuilder_;
};

} // namespace sf1r

#endif // RECOMMEND_SEARCH_SERVICE_H
