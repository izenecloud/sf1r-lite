/**
 * @file process/controllers/KeywordsController.cpp
 * @author Ian Yang
 * @date Created <2010-06-01 16:04:55>
 */
#include "KeywordsController.h"
#include "CollectionHandler.h"

#include <common/Keys.h>

#include <bundles/mining/MiningSearchService.h>

#include <log-manager/LogManager.h>
#include <log-manager/LogAnalysis.h>

namespace sf1r
{

using driver::Keys;
using namespace izenelib::driver;
/**
 * @brief Action \b index. Gets various keywords list.
 *
 * @section request
 *
 * - @b collection* (@c String): Collection name.
 * - @b limit (@c Uint = @ref kDefaultCount): Limit the count of keywords in
 *   each list.
 * - @b select (@c Array): Select from following list types:
 *   - @b recent : recently used keywords
 *   - @b realtime : realtime keywords
 *   - @b popular : popular keywords
 *   Default is getting all types of keywords list.
 *
 * @section response
 *
 * Each keywords list is returned as an Array of strings. The key is the list
 * type such as recent, realtime or popular.
 *
 * @section Example
 *
 * Request
 * @code
 * {
 *   "collection": "EngWiki",
 *   "limit": 3,
 *   "select": ["recent"]
 * }
 * @endcode
 *
 * Response
 * @code
 * {
 *   "header": {"success": true},
 *   "recent": [
 *     "sports",
 *     "data mining",
 *     "algorithm"
 *   ]
 * }
 * @endcode
 */
void KeywordsController::index()
{
    Value::StringType collection = asString(request()[Keys::collection]);
    Value::UintType limit = asUintOr(request()[Keys::limit], kDefaultCount);

    if (collection.empty())
    {
        response().addError("Require field collection in request.");
        return;
    }

    bool getAllLists = true;
    std::set<std::string> selectedTypes;
    if (!nullValue(request()[Keys::select]))
    {
        getAllLists = false;
        const Value::ArrayType* selectArray =
            request()[Keys::select].getPtr<Value::ArrayType>();
        if (selectArray)
        {
            for (std::size_t i = 0; i < selectArray->size(); ++i)
            {
                std::string type = asString((*selectArray)[i]);
                boost::to_lower(type);
                selectedTypes.insert(type);
            }
        }
    }

    std::string convertBuffer;
    const izenelib::util::UString::EncodingType kEncoding =
        izenelib::util::UString::UTF_8;
    typedef std::vector<izenelib::util::UString>::const_iterator iterator;

    // recent
    if (getAllLists || selectedTypes.count(Keys::recent))
    {
        Value& recentField = response()[Keys::recent];
        recentField.reset<Value::ArrayType>();
        std::vector<izenelib::util::UString> recentKeywords;
        LogAnalysis::getRecentKeywordList(collection, limit, recentKeywords);

        // set resource
        for (iterator it = recentKeywords.begin(); it != recentKeywords.end(); ++it)
        {
            it->convertString(convertBuffer, kEncoding);
            recentField() = convertBuffer;
        }

//         LogManager::LogManagerPtr logManager = &LogManager::instance();
//         if (logManager)
//         {
//
//         }
//         else
//         {
//             response().addWarning(
//                 "Cannot find recent keywords log for given collection."
//             );
//         }
    }

    // realtime and popular
    if (getAllLists ||
            selectedTypes.count(Keys::popular) ||
            selectedTypes.count(Keys::realtime))
    {
        std::vector<izenelib::util::UString> popularKeywords;
        std::vector<izenelib::util::UString> realtimeKeywords;

        MiningSearchService* service = collectionHandler_->miningSearchService_;

        bool success = service->getReminderQuery(popularKeywords, realtimeKeywords);

        if (getAllLists || selectedTypes.count(Keys::popular))
        {
            Value& popularField = response()[Keys::popular];
            popularField.reset<Value::ArrayType>();

            if (success)
            {
                if (popularKeywords.size() > limit)
                {
                    popularKeywords.resize(limit);
                }

                // set resource
                for (iterator it = popularKeywords.begin(); it != popularKeywords.end(); ++it)
                {
                    it->convertString(convertBuffer, kEncoding);
                    popularField() = convertBuffer;
                }
            }
            else
            {
                response().addWarning(
                    "Cannot get popular keywords from MIA"
                );
            }
        }

        if (getAllLists || selectedTypes.count(Keys::realtime))
        {
            Value& realtimeField = response()[Keys::realtime];
            realtimeField.reset<Value::ArrayType>();

            if (success)
            {
                if (realtimeKeywords.size() > limit)
                {
                    realtimeKeywords.resize(limit);
                }

                // set resource
                for (iterator it = realtimeKeywords.begin(); it != realtimeKeywords.end(); ++it)
                {
                    it->convertString(convertBuffer, kEncoding);
                    realtimeField() = convertBuffer;
                }
            }
            else
            {
                response().addWarning(
                    "Cannot get realtime keywords from MIA"
                );
            }
        }
    }
}

} // namespace sf1r
