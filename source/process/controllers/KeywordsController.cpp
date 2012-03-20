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

#include <cache/IzeneCache.h>

namespace sf1r
{

using driver::Keys;
using namespace izenelib::driver;

KeywordsController::KeywordsController()
    : Sf1Controller(false)
    , miningSearchService_(NULL)
{
}

bool KeywordsController::checkCollectionService(std::string& error)
{
    miningSearchService_ = collectionHandler_->miningSearchService_;

    if (miningSearchService_)
        return true;

    error = "Request failed, no mining search service found.";
    return false;
}

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
    IZENELIB_DRIVER_BEFORE_HOOK(checkCollectionName());

    Value::UintType limit = asUintOr(request()[Keys::limit], kDefaultCount);
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
        typedef std::pair<std::vector<izenelib::util::UString>, time_t > keyword_cache_value_type;
        typedef izenelib::cache::IzeneCache<
            std::string,
            keyword_cache_value_type,
            izenelib::util::ReadWriteLock,
            izenelib::cache::RDE_HASH,
            izenelib::cache::LRLFU
        > keyword_cache_type;
        static keyword_cache_type recent_keyword_cache;
        static const time_t refreshInterval = 30;

        Value& recentField = response()[Keys::recent];
        recentField.reset<Value::ArrayType>();
        keyword_cache_value_type recentKeywords;
        if (!recent_keyword_cache.getValueNoInsert(collectionName_, recentKeywords)
                || ((std::time(NULL) - recentKeywords.second) > refreshInterval) )
        {
            LogAnalysis::getRecentKeywordList(collectionName_, limit, recentKeywords.first);
            if(!recentKeywords.first.empty())
            {
                recentKeywords.second = std::time(NULL);
                recent_keyword_cache.updateValue(collectionName_,recentKeywords);
            }
        }

        // set resource
        for (iterator it = recentKeywords.first.begin(); it != recentKeywords.first.end(); ++it)
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

        bool success = miningSearchService_->getReminderQuery(popularKeywords, realtimeKeywords);

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
        if (collectionName_.empty())
        {
            for(uint32_t i=0;i<input.size();i++)
            {
                QueryCorrectionSubmanager::getInstance().Inject(input[i].first, input[i].second);
            }
            QueryCorrectionSubmanager::getInstance().FinishInject();
        }
        else
        {
            for(uint32_t i=0;i<input.size();i++)
            {
                miningSearchService_->InjectQueryCorrection(input[i].first, input[i].second);
            }
            miningSearchService_->FinishQueryCorrectionInject();
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
    IZENELIB_DRIVER_BEFORE_HOOK(checkCollectionName());

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
        for(uint32_t i=0;i<input.size();i++)
        {
            miningSearchService_->InjectQueryRecommend(input[i].first, input[i].second);
        }
        miningSearchService_->FinishQueryRecommendInject();
    }

}

} // namespace sf1r
