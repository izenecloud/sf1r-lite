#include "QueryCorrectionController.h"
#include "CollectionHandler.h"

#include <common/Keys.h>
#include <common/CollectionManager.h>
#include <bundles/mining/MiningSearchService.h>
#include <mining-manager/query-correction-submanager/QueryCorrectionSubmanager.h>

#include <util/ustring/UString.h>

#include <vector>
#include <string>

namespace sf1r
{

using driver::Keys;

/**
 * @brief Action \b index. Refines a user query which can be a set of tokens.
 *
 * @section request
 *
 * - @b collection* (@c String): Collection name.
 * - @b keywords* (@c String): Specified query string.
 *
 * @section response
 *
 *   specified prefix.
 * - @b refined_query (@c String): A refined query string.
 *
 * @section Example
 *
 * Request
 * @code
 * {
 *   "collection": "example",
 *   "keywords": "shanghai yiyao"
 * }
 * @endcode
 *
 * Response
 * @code
 * {
 *   "header": {"success": true},
 *   "refined_query": "上海 医药"
 * }
 * @endcode
 */
void QueryCorrectionController::index()
{
    std::string collection = asString(request()[Keys::collection]);
    std::string queryString = asString(request()[Keys::keywords]);

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
        CollectionManager::MutexType* mutex = CollectionManager::get()->getCollectionMutex(collection);
        CollectionManager::ScopedReadLock lock(*mutex);
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
