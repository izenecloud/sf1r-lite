#ifndef MINING_BUNDLE_SEARCH_SERVICE_H
#define MINING_BUNDLE_SEARCH_SERVICE_H

#include "MiningBundleConfiguration.h"

#include <util/osgi/IService.h>

#include <common/sf1_serialization_types.h>
#include <common/type_defs.h>

#include <aggregator-manager/SearchAggregator.h>
#include <query-manager/ActionItem.h>
#include <mining-manager/faceted-submanager/manmade_doc_category_item.h>
#include <mining-manager/summarization-submanager/Summarization.h>
#include <mining-manager/custom-rank-manager/CustomRankValue.h>
#include <mining-manager/merchant-score-manager/MerchantScore.h>
#include <idmlib/tdt/tdt_types.h>
#include <boost/shared_ptr.hpp>

namespace sf1r
{

class SearchWorker;
class MiningManager;
class MiningSearchService : public ::izenelib::osgi::IService
{
public:
    MiningSearchService();

    ~MiningSearchService();

public:
    /// distributed search
    bool getSimilarDocIdList(
            const std::string& collectionName,
            uint64_t documentId,
            uint32_t maxNum,
            std::vector<std::pair<uint64_t, float> >& result);

    bool visitDoc(
            const std::string& collectionName,
            uint64_t wdocId);

    bool getDocLabelList(
            const std::string& collectionName,
            uint32_t docid,
            std::vector<std::pair<uint32_t, izenelib::util::UString> >& label_list );

    bool getLabelListWithSimByDocId(
            const std::string& collectionName,
            uint32_t docid,
            std::vector<std::pair<izenelib::util::UString, std::vector<izenelib::util::UString> > >& label_list);

public:
    bool getSearchResult(const KeywordSearchActionItem& actionItem, KeywordSearchResult& resultItem);

    bool getSimilarDocIdList(uint32_t documentId, uint32_t maxNum, std::vector<std::pair<uint32_t, float> >& result);

    bool getDuplicateDocIdList(uint32_t docId, std::vector<uint32_t>& docIdList);

    bool getSimilarImageDocIdList(const std::string& targetImageURI, SimilarImageDocIdList& imageDocIdList);

    bool getReminderQuery(std::vector<izenelib::util::UString>& popularQueries, std::vector<izenelib::util::UString>& realtimeQueries);

    bool getDocLabelList(uint32_t docid, std::vector<std::pair<uint32_t, izenelib::util::UString> >& label_list );

    bool getSimilarLabelStringList(uint32_t label_id, std::vector<izenelib::util::UString>& label_list );

    bool getLabelListWithSimByDocId(uint32_t docid,  std::vector<std::pair<izenelib::util::UString, std::vector<izenelib::util::UString> > >& label_list);

    bool getUniqueDocIdList(const std::vector<uint32_t>& docIdList, std::vector<uint32_t>& cleanDocs);

    ///faceted api
    bool SetOntology(const std::string& xml);

    bool GetOntology(std::string& xml);

    bool GetStaticOntologyRepresentation(faceted::OntologyRep& rep);

    bool OntologyStaticClick(uint32_t cid, std::list<uint32_t>& docid_list);

    bool GetOntologyRepresentation(const std::vector<uint32_t>& search_result, faceted::OntologyRep& rep);

    bool OntologyClick(const std::vector<uint32_t>& search_result, uint32_t cid, std::list<uint32_t>& docid_list);

    bool DefineDocCategory(const std::vector<faceted::ManmadeDocCategoryItem>& items);

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

    bool GetTdtInTimeRange(const izenelib::util::UString& start, const izenelib::util::UString& end, std::vector<izenelib::util::UString>& topic_list);
    bool GetTdtTopicInfo(const izenelib::util::UString& text, idmlib::tdt::TopicInfoType& topic_info);
    bool GetTopics(const std::string& content, std::vector<std::string>& topic_list, size_t limit);

    boost::shared_ptr<MiningManager> GetMiningManager() const
    {
        return miningManager_;
    }

    void GetRefinedQuery(const izenelib::util::UString& query, izenelib::util::UString& result);

    void InjectQueryCorrection(const izenelib::util::UString& query, const izenelib::util::UString& result);

    void FinishQueryCorrectionInject();

    void InjectQueryRecommend(const izenelib::util::UString& query, const izenelib::util::UString& result);

    void FinishQueryRecommendInject();

    bool GetSummarizationByRawKey(
            const std::string& collection,
            const izenelib::util::UString& rawKey,
            Summarization& result);

    bool SetKV(const std::string& key, const std::string& value);
    bool GetKV(const std::string& key, std::string& value);

    bool GetProductScore(
            const std::string& docIdStr,
            const std::string& scoreType,
            score_t& scoreValue);

    bool GetProductCategory(
            const std::string& query,
            int limit,
            std::vector<std::vector<std::string> >& pathVec);

private:
    bool HookDistributeRequestForSearch(const std::string& coll, uint32_t workerId);
    MiningBundleConfiguration* bundleConfig_;
    boost::shared_ptr<MiningManager> miningManager_;

    boost::shared_ptr<SearchWorker> searchWorker_;
    boost::shared_ptr<SearchAggregator> searchAggregator_;

    friend class MiningBundleActivator;
};

}

#endif
