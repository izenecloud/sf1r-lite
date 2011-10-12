#include "QueryLogSearchService.h"

#include <mining-manager/query-correction-submanager/QueryCorrectionSubmanager.h>


namespace sf1r
{

QueryLogSearchService::QueryLogSearchService()
{
}

QueryLogSearchService::~QueryLogSearchService()
{
}

bool QueryLogSearchService::getRefinedQuery(
    const std::string& collectionName,
    const UString& queryUString,
    UString& refinedQueryUString
)
{
    return QueryCorrectionSubmanager::getInstance().getRefinedQuery(
			collectionName, queryUString,
			refinedQueryUString);
}

// bool QueryLogSearchService::getAutoFillList(
//     const izenelib::util::UString& query,
//     std::vector<std::pair<izenelib::util::UString,uint32_t> >& list
// )
// {
//     AutoFillSubManager::get()->getAutoFillList(query, list);
//     return true;
// }


}

