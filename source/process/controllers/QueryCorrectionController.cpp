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

QueryCorrectionController::QueryCorrectionController()
    : Sf1Controller(false)
    , miningSearchService_(NULL)
{
}

bool QueryCorrectionController::checkCollectionService(std::string& error)
{
    miningSearchService_ = collectionHandler_->miningSearchService_;

    if (miningSearchService_)
        return true;

    error = "Request failed, no mining search service found.";
    return false;
}

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
    std::string queryString = asString(request()[Keys::keywords]);

    UString queryUString(queryString, UString::UTF_8);
    UString refinedQueryUString;

    if (collectionName_.empty())
    {
        QueryCorrectionSubmanager::getInstance().getRefinedQuery(
                queryUString,
                refinedQueryUString);
    }
    else
    {
        miningSearchService_->GetRefinedQuery(queryUString, refinedQueryUString);
    }

    std::string refinedQueryString;
    refinedQueryUString.convertString(refinedQueryString,
                                     izenelib::util::UString::UTF_8);
    response()[Keys::refined_query] = refinedQueryString;
}

} // namespace sf1r
