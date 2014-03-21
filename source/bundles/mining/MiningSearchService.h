#ifndef MINING_BUNDLE_SEARCH_SERVICE_H
#define MINING_BUNDLE_SEARCH_SERVICE_H

#include "MiningBundleConfiguration.h"

#include <util/osgi/IService.h>

#include <common/sf1_serialization_types.h>
#include <common/type_defs.h>

#include <aggregator-manager/SearchAggregator.h>
#include <query-manager/ActionItem.h>
#include <mining-manager/custom-rank-manager/CustomRankValue.h>
#include <mining-manager/merchant-score-manager/MerchantScore.h>
#include <boost/shared_ptr.hpp>

namespace sf1r
{

class SearchWorker;
class MiningManager;
class ShardingStrategy;

class MiningSearchService : public ::izenelib::osgi::IService
{
public:
    MiningSearchService();

    ~MiningSearchService();

public:
    /// distributed search
    bool visitDoc(
            const std::string& collectionName,
            uint64_t wdocId);

    bool getSearchResult(const KeywordSearchActionItem& actionItem, KeywordSearchResult& resultItem);

    bool visitDoc(const uint128_t& scdDocId);

    /**
     * Log the group label click.
     * @param query user query
     * @param propName the property name of the group label
     * @param groupPath the path of the group label
     * @return true for success, false for failure
     */
    bool clickGroupLabel(
            const std::string& query,
            const std::string& propName,
            const std::vector<std::string>& groupPath);

    /**
     * Get the most frequently clicked group labels.
     * @param query user query
     * @param propName the property name for the group labels to get
     * @param limit the max number of labels to get
     * @param pathVec store the path for each group label
     * @param freqVec the click count for each group label
     * @return true for success, false for failure
     * @post @p freqVec is sorted in descending order.
     */
    bool getFreqGroupLabel(
            const std::string& query,
            const std::string& propName,
            int limit,
            std::vector<std::vector<std::string> >& pathVec,
            std::vector<int>& freqVec);

    /**
     * Set the most frequently clicked group labels.
     * @param query user query
     * @param propName the property name
     * @param groupLabels an array of group paths
     * @return true for success, false for failure
     */
    bool setTopGroupLabel(
            const std::string& query,
            const std::string& propName,
            const std::vector<std::vector<std::string> >& groupLabels);

    /**
     * Set custom ranking for @p query.
     * @param query user query
     * @param customDocStr custom rankings
     * @return true for success, false for failure
     */
    bool setCustomRank(
            const std::string& query,
            const CustomRankDocStr& customDocStr);

    /**
     * Get the custom ranking for @p query.
     * @param query user query
     * @param topDocList doc list for top ranking docs
     * @param excludeDocList doc list for docs to exclude
     * @return true for success, false for failure
     */
    bool getCustomRank(
            const std::string& query,
            std::vector<Document>& topDocList,
            std::vector<Document>& excludeDocList);

    /**
     * Get @p queries which have been customized by @c setCustomRank() with
     * non-empty doc id list.
     * @return true for success, false for failure
     */
    bool getCustomQueries(std::vector<std::string>& queries);

    bool setMerchantScore(const MerchantStrScoreMap& scoreMap);

    bool getMerchantScore(
            const std::vector<std::string>& merchantNames,
            MerchantStrScoreMap& merchantScoreMap) const;

    boost::shared_ptr<MiningManager> GetMiningManager() const
    {
        return miningManager_;
    }

    bool GetProductScore(
            const std::string& docIdStr,
            const std::string& scoreType,
            score_t& scoreValue);

    bool GetProductCategory(
            const std::string& query,
            int limit,
            std::vector<std::vector<std::string> >& pathVec);

    void setShardingStrategy(boost::shared_ptr<ShardingStrategy> shardingstrategy)
    {
        sharding_strategy_ = shardingstrategy;
    }

private:
    bool HookDistributeRequestForSearch(const std::string& coll, uint32_t workerId);
    MiningBundleConfiguration* bundleConfig_;
    boost::shared_ptr<MiningManager> miningManager_;

    boost::shared_ptr<SearchWorker> searchWorker_;
    boost::shared_ptr<SearchAggregator> searchAggregator_;
    boost::shared_ptr<SearchAggregator> ro_searchAggregator_;
    boost::shared_ptr<ShardingStrategy> sharding_strategy_;

    friend class MiningBundleActivator;
};

}

#endif
