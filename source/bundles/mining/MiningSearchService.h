#ifndef MINING_BUNDLE_SEARCH_SERVICE_H
#define MINING_BUNDLE_SEARCH_SERVICE_H

#include <util/osgi/IService.h>

#include <common/sf1_serialization_types.h>
#include <common/type_defs.h>

#include <query-manager/ActionItem.h>
#include <mining-manager/MiningManager.h>

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

    bool getGroupRep(
        const std::vector<unsigned int>& docIdList,
        const std::vector<std::string>& groupPropertyList,
        const std::vector<std::pair<std::string, std::string> >& groupLabelList,
        faceted::OntologyRep& groupRep,
        bool isAttrGroup,
        int attrGroupNum,
        const std::vector<std::pair<std::string, std::string> >& attrLabelList,
        faceted::OntologyRep& attrRep
    );
    
    bool GetTdtInTimeRange(const izenelib::util::UString& start, const izenelib::util::UString& end, std::vector<izenelib::util::UString>& topic_list);
    bool GetTdtTopicInfo(const izenelib::util::UString& text, idmlib::tdt::TopicInfoType& topic_info);
    
    

private:
    boost::shared_ptr<MiningManager> miningManager_;

    friend class MiningBundleActivator;
};

}

#endif

