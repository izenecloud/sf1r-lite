#ifndef MINING_BUNDLE_SEARCH_SERVICE_H
#define MINING_BUNDLE_SEARCH_SERVICE_H

#include <util/osgi/IService.h>

#include <common/sf1_serialization_types.h>
#include <common/type_defs.h>

#include <query-manager/ActionItem.h>
#include <mining-manager/MiningManager.h>
#include <mining-manager/faceted-submanager/manmade_doc_category_item.h>
#include <idmlib/tdt/tdt_types.h>
#include <boost/shared_ptr.hpp>

namespace sf1r
{
class MiningSearchService : public ::izenelib::osgi::IService
{
public:
    MiningSearchService();

    ~MiningSearchService();

public:
    bool getSearchResult(KeywordSearchResult& resultItem);

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

    /// visit document
    bool visitDoc(uint32_t docId);

    /**
     * Log the group label click.
     * @param query user query
     * @param propName the property name of the group label
     * @param propValue the property value of the group label
     * @return true for success, false for failure
     */
    bool clickGroupLabel(
        const std::string& query,
        const std::string& propName,
        const std::string& propValue
    );

    /**
     * Get the most frequently clicked group labels.
     * @param query user query
     * @param propName the property name for the group labels to get
     * @param limit the max number of labels to get
     * @param propValueVec store the property values of each group label
     * @param freqVec the click count for each group label
     * @return true for success, false for failure
     * @post @p freqVec is sorted in descending order.
     */
    bool getFreqGroupLabel(
        const std::string& query,
        const std::string& propName,
        int limit,
        std::vector<std::string>& propValueVec,
        std::vector<int>& freqVec
    );

    /**
     * Set the most frequently clicked group label.
     * @param query user query
     * @param propName the property name for the group labels to get
     * @param propValue the property value of group label
     * @return true for success, false for failure
     */
    bool setTopGroupLabel(
        const std::string& query,
        const std::string& propName,
        const std::string& propValue
    );

    bool GetTdtInTimeRange(const izenelib::util::UString& start, const izenelib::util::UString& end, std::vector<izenelib::util::UString>& topic_list);
    bool GetTdtTopicInfo(const izenelib::util::UString& text, idmlib::tdt::TopicInfoType& topic_info);
    
    boost::shared_ptr<MiningManager> GetMiningManager() const
    {
        return miningManager_;
    }

    void InjectQueryRecommend(const izenelib::util::UString& query, const izenelib::util::UString& result);
    
    void FinishQueryRecommendInject();
    

private:
    boost::shared_ptr<MiningManager> miningManager_;

    friend class MiningBundleActivator;
};

}

#endif

