#include "QueryLogSearchService.h"

#include <mining-manager/query-correction-submanager/QueryCorrectionSubmanager.h>


namespace sf1r
{

QueryLogSearchService *QueryLogSearchService::serviceInstance_ = NULL;

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

QueryLogSearchService *QueryLogSearchService::instance()
{
    if (!serviceInstance_)
        serviceInstance_ = new QueryLogSearchService();

    return serviceInstance_;
}

}
