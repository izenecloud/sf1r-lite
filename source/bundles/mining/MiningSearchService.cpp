#include "MiningSearchService.h"

namespace sf1r
{

MiningSearchService::MiningSearchService()
{
}

MiningSearchService::~MiningSearchService()
{
}


bool MiningSearchService::getSearchResult(
    KeywordSearchResult& resultItem
)
{
    return true;
}

bool MiningSearchService::getSimilarDocIdList(
    uint32_t documentId, 
    uint32_t maxNum, 
    std::vector<std::pair<uint32_t, float> >& result
)
{
    return true;
}

bool MiningSearchService::getDuplicateDocIdList(
    uint32_t docId, 
    std::vector<uint32_t>& docIdList
)
{
    return true;
}

bool MiningSearchService::getSimilarImageDocIdList(
    const std::string& targetImageURI,
    SimilarImageDocIdList& imageDocIdList
)
{
    return true;
}

bool MiningSearchService::getReminderQuery(
    std::vector<izenelib::util::UString>& popularQueries, 
    std::vector<izenelib::util::UString>& realtimeQueries
)
{
    return true;
}

bool MiningSearchService::getSimilarLabelStringList(
    uint32_t label_id, 
    std::vector<izenelib::util::UString>& label_list 
)
{
    return true;
}

bool MiningSearchService::SetOntology(const std::string& xml)
{
    return true;
}

bool MiningSearchService::GetOntology(std::string& xml)
{
    return true;
}

bool MiningSearchService::GetStaticOntologyRepresentation(faceted::OntologyRep& rep)
{
    return true;
}

bool MiningSearchService::OntologyStaticClick(uint32_t cid, std::list<uint32_t>& docid_list)
{
    return true;
}

bool MiningSearchService::GetOntologyRepresentation(
    const std::vector<uint32_t>& search_result, 
    faceted::OntologyRep& rep
)
{
    return true;
}

bool MiningSearchService::OntologyClick(
    const std::vector<uint32_t>& search_result, 
    uint32_t cid, 
    std::list<uint32_t>& docid_list
)
{
    return true;
}

bool MiningSearchService::DefineDocCategory(
    const std::vector<faceted::ManmadeDocCategoryItem>& items
)
{
    return true;
}

}

