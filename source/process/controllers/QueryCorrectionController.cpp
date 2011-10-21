#include "QueryCorrectionController.h"
#include "CollectionHandler.h"

#include <common/Keys.h>
#include <common/XmlConfigParser.h>
#include <common/CollectionManager.h>
#include <bundles/mining/MiningSearchService.h>

#include <mining-manager/query-correction-submanager/QueryCorrectionSubmanager.h>

#include <util/ustring/UString.h>

#include <vector>
#include <string>

namespace sf1r
{

using driver::Keys;

void QueryCorrectionController::index()
{
    std::string collection = asString(request()[Keys::collection]);
    std::string queryString = asString( request()[Keys::keywords] );

    UString queryUString(queryString, UString::UTF_8);
    UString refinedQueryUString;

    if (collection.empty())
    {
        QueryCorrectionSubmanager::getInstance().getRefinedQuery(
                queryUString,
                refinedQueryUString);
    }
    else
    {
        if (!SF1Config::get()->checkCollectionAndACL(collection, request().aclTokens()))
        {
            response().addError("Collection access denied");
            return;
        }
        CollectionHandler* collectionHandler = CollectionManager::get()->findHandler(collection);
        MiningSearchService* service = collectionHandler->miningSearchService_;
        service->GetRefinedQuery(queryUString, refinedQueryUString);
    }

    std::string refinedQueryString;
    refinedQueryUString.convertString(refinedQueryString,
                                     izenelib::util::UString::UTF_8);
    response()[Keys::refined_query] = refinedQueryString;
}

} // namespace sf1r
