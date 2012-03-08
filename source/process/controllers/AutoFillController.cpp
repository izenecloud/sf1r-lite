/**
 * @file process/controllers/AutoFillController.cpp
 * @author Ian Yang
 * @date Created <2010-05-31 14:13:29>
 */
#include "AutoFillController.h"
#include "CollectionHandler.h"

#include <bundles/mining/MiningSearchService.h>
#include <mining-manager/query-recommend-submanager/RecommendManager.h>
#include <mining-manager/MiningManager.h>

#include <common/Keys.h>

#include <util/ustring/UString.h>

#include <vector>
#include <string>

namespace sf1r
{

using driver::Keys;
using namespace izenelib::driver;

bool AutoFillController::checkCollectionService(std::string& error)
{
    MiningSearchService* service = collectionHandler_->miningSearchService_;
    if (!service)
    {
        error = "Request failed, no mining search service found.";
        return false;
    }

    boost::shared_ptr<MiningManager> mining_manager = service->GetMiningManager();
    recommend_manager_ = mining_manager->GetRecommendManager();
    if (!recommend_manager_)
    {
        error = "Request failed, RecommendManager not enabled.";
        return false;
    }

    return true;
}

/**
 * @brief Action \b index. Gets list of logged keywords starting with specified prefix.
 *
 * @section request
 *
 * - @b collection* (@c String): Collection name.
 * - @b prefix* (@c String): Specified keywords prefix.
 * - @b limit (@c Uint = @ref kDefaultCount): Limit the count of result in
 *   response. If the limit is larger than @ref kMaxCount, @ref kMaxCount is
 *   used instead.
 *
 * @section response
 *
 * - @b total_count (@c Uint): Total count of logged keywords starting with the
 *   specified prefix.
 * - @b keywords (@c Array): An array of strings with its document frequency. 
 *   - @b name (@c String): The string text.
 *   - @b count (@c Uint): Document frequency.
 *
 * @section Example
 *
 * Request
 * @code
 * {
 *   "collection": "example",
 *   "prefix": "auto",
 *   "limit": 10
 * }
 * @endcode
 *
 * Response
 * @code
 * {
 *   "header": {"success": true},
 *   "keywords": [
 *     { "name" : "autocad", "count" : 20 } ,
 *     { "name" : "autodesk", "count" : 10 } ,
 *     { "name" : "autoexec", "count" : 8 } ,
 *     { "name" : "automatic", "count" : 15 } ,
 *     { "name" : "automobile", "count" : 12 }
 *   ]
 * }
 * @endcode
 */
void AutoFillController::index()
{
    Value::StringType prefix = asString(request()[Keys::prefix]);
    Value::UintType limit = asUintOr(request()[Keys::limit], kDefaultCount);
    if (limit > kMaxCount)
    {
        limit = kMaxCount;
    }

    if (prefix.empty())
    {
        response().addError("Require field prefix in request.");
        return;
    }

    izenelib::util::UString query(prefix, izenelib::util::UString::UTF_8);
    std::vector<std::pair<izenelib::util::UString, uint32_t> > result;
    recommend_manager_->getAutoFillList(query, result);

    Value::UintType count = result.size();
    if (limit < count)
    {
        count = limit;
    }

    response()[Keys::total_count] = result.size();

    // set resource
    Value& keywords = response()[Keys::keywords];
    std::string convertBuffer;
    for (Value::UintType i = 0; i < count; ++i)
    {
        result[i].first.convertString(convertBuffer, izenelib::util::UString::UTF_8);
        Value& item = keywords();
        item[Keys::name] = convertBuffer;
        item[Keys::count] = result[i].second;
//         keywords() = convertBuffer;
    }
}

} // namespace sf1r
