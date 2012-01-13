/**
 * @file process/controllers/KeywordsController.cpp
 * @author Ian Yang
 * @date Created <2010-06-01 16:04:55>
 */
#include "KeywordsController.h"
#include "CollectionHandler.h"

#include <common/Keys.h>
#include <common/XmlConfigParser.h>
#include <common/CollectionManager.h>
#include <bundles/mining/MiningSearchService.h>
#include <mining-manager/query-correction-submanager/QueryCorrectionSubmanager.h>
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
    std::string collection = asString(request()[Keys::collection]);
    Value::UintType limit = asUintOr(request()[Keys::limit], kDefaultCount);

    if (collection.empty())
    {
        response().addError("Require field collection in request.");
        return;
    }
    if (!SF1Config::get()->checkCollectionAndACL(collection, request().aclTokens()))
    {
        response().addError("Collection access denied");
        return;
    }

    CollectionManager::MutexType* mutex = CollectionManager::get()->getCollectionMutex(collection);
    CollectionManager::ScopedReadLock lock(*mutex);
    CollectionHandler* collectionHandler = CollectionManager::get()->findHandler(collection);

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
    }

    // realtime and popular
    if (getAllLists ||
            selectedTypes.count(Keys::popular) ||
            selectedTypes.count(Keys::realtime))
    {
        std::vector<izenelib::util::UString> popularKeywords;
        std::vector<izenelib::util::UString> realtimeKeywords;

        MiningSearchService* service = collectionHandler->miningSearchService_;

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


/**
 * @brief Action \b inject_query_correction. inject query correction result.
 *
 * @section request
 *
 * - @b collection (@c String): collection name.
 * - @b resource* (@c Array): all inject queries.
 *   - @b query (@c String) : inject query
 *   - @b result (@c String) : inject result to query, or '__delete__' meaning delete the injection of this query.
 *
 * @section response
 *
 * whether it is success.
 *
 * @section Example
 *
 * Request
 * @code
 * {
 *   "collection" : "intel",
 *   "resource": [
 *       { "query" : "chinesa", "result" : "Chinese" },
 *       { "query" : "iphone", "result" : "" }
 *   ]
 * }
 * @endcode
 *
 * Response
 * @code
 * {
 *   "header": {"success": true}
 * }
 * @endcode
 */
void KeywordsController::inject_query_correction()
{
    std::string collection = asString(request()[Keys::collection]);
    Value& resources = request()[Keys::resource];
    std::vector<std::pair<izenelib::util::UString,izenelib::util::UString> > input;
    for(uint32_t i=0;i<resources.size();i++)
    {
        Value& resource = resources(i);
        std::string str_query = asString(resource[Keys::query]);
        if (str_query.empty())
        {
            response().addError("Require field query in request.");
            return;
        }
        std::string str_result = asString(resource[Keys::result]);
//         if (str_result.empty())
//         {
//             response().addError("Require field result in request.");
//             return;
//         }
        izenelib::util::UString query(str_query, izenelib::util::UString::UTF_8);
        izenelib::util::UString result(str_result, izenelib::util::UString::UTF_8);
        input.push_back(std::make_pair(query, result) );

    }
    if(!input.empty())
    {
        if (collection.empty())
        {
            for(uint32_t i=0;i<input.size();i++)
            {
                QueryCorrectionSubmanager::getInstance().Inject(input[i].first, input[i].second);
            }
            QueryCorrectionSubmanager::getInstance().FinishInject();
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
            for(uint32_t i=0;i<input.size();i++)
            {
                service->InjectQueryCorrection(input[i].first, input[i].second);
            }
            service->FinishQueryCorrectionInject();
       }
    }
}


/**
 * @brief Action \b inject_query_recommend. inject query recommend result.
 *
 * @section request
 *
 * - @b collection* (@c String): collection name.
 * - @b resource* (@c Array): all inject queries.
 *   - @b query (@c String) : inject query
 *   - @b result (@c String) : results to query splited by '|', or '__delete__' meaning delete the injection of this query.
 *
 * @section response
 *
 * whether it is success.
 *
 * @section Example
 *
 * Request
 * @code
 * {
 *   "collection" : "intel",
 *   "resource": [
 *       { "query" : "canon", "result" : "canon printer|canon d600" },
 *       { "query" : "iphone", "result" : "iphone4" }
 *   ]
 * }
 * @endcode
 *
 * Response
 * @code
 * {
 *   "header": {"success": true}
 * }
 * @endcode
 */
void KeywordsController::inject_query_recommend()
{
    std::string collection = asString(request()[Keys::collection]);
    if (collection.empty())
    {
        response().addError("Require field collection in request.");
        return;
    }
    if (!SF1Config::get()->checkCollectionAndACL(collection, request().aclTokens()))
    {
        response().addError("Collection access denied");
        return;
    }
    Value& resources = request()[Keys::resource];
    std::vector<std::pair<izenelib::util::UString,izenelib::util::UString> > input;
    for(uint32_t i=0;i<resources.size();i++)
    {
        Value& resource = resources(i);
        std::string str_query = asString(resource[Keys::query]);
        if (str_query.empty())
        {
            response().addError("Require field query in request.");
            return;
        }
        std::string str_result = asString(resource[Keys::result]);
//         if (str_result.empty())
//         {
//             response().addError("Require field result in request.");
//             return;
//         }
        izenelib::util::UString query(str_query, izenelib::util::UString::UTF_8);
        izenelib::util::UString result(str_result, izenelib::util::UString::UTF_8);
        input.push_back(std::make_pair(query, result) );

    }
    if(!input.empty())
    {
        CollectionManager::MutexType* mutex = CollectionManager::get()->getCollectionMutex(collection);
        CollectionManager::ScopedReadLock lock(*mutex);
        CollectionHandler* collectionHandler = CollectionManager::get()->findHandler(collection);
        MiningSearchService* service = collectionHandler->miningSearchService_;
        for(uint32_t i=0;i<input.size();i++)
        {
            service->InjectQueryRecommend(input[i].first, input[i].second);
        }
        service->FinishQueryRecommendInject();
    }

}

} // namespace sf1r
