#include "TopicController.h"
#include "CollectionHandler.h"

#include <common/Keys.h>

#include <bundles/mining/MiningSearchService.h>
#include <idmlib/util/time_util.h>
namespace sf1r
{
using driver::Keys;
using namespace izenelib::driver;

TopicController::TopicController()
    : miningSearchService_(NULL)
{
}

bool TopicController::checkCollectionService(std::string& error)
{
    miningSearchService_ = collectionHandler_->miningSearchService_;

    if (miningSearchService_)
        return true;

    error = "Request failed, no mining search service found.";
    return false;
}

/**
 * @brief Action @b get_similar. Get similar topics from one topic id
 *
 * @section request
 *
 * - @b collection* (@c String): Find similar topics in this collection.
 * - @b similar_to* (@c Object): Specify the target topic id.
 *   - @c {"similar_to":{"id":1}} get topics' names similar to the one with tid=1
 *
 * @section response
 *
 * - @b resources (@c Array): Every item is a topic.
 *   - @b name (@c String): The name of topic
 *
 *
 * @section example
 *
 * @code
 * {
 *   "collection": "ChnWiki",
 *   "similar_to" : {
 *     "id": "1",
 *   }
 * }
 * @endcode
 */
void TopicController::get_similar()
{
//   std::cout<<"TopicController::get_similar"<<std::endl;
    IZENELIB_DRIVER_BEFORE_HOOK(requireTID_());
    std::vector<izenelib::util::UString> label_list;
    bool requestSent = miningSearchService_->getSimilarLabelStringList(tid_, label_list);

    if (!requestSent)
    {
        response().addError(
            "Request Failed."
        );
        return;
    }
//   std::cout<<"find similar : "<<label_list.size()<<std::endl;
    Value& resources = response()[Keys::resources];
    resources.reset<Value::ArrayType>();
    for (uint32_t i=0;i<label_list.size();i++)
    {
        Value& new_resource = resources();
        std::string str;
        label_list[i].convertString(str, izenelib::util::UString::UTF_8);
        new_resource[Keys::name] = str;

    }
}

/**
 * @brief Action @b get_in_date_range. Get topics bursting in specific date range
 *
 * @section request
 *
 * - @b collection* (@c String): Find topics in this collection.
 * - @b date_range* (@c Object): Specify the date range
 *   - @c "start": (@c String): start date string
 *   - @c "end": (@c String): end date string
 *
 * @section response
 *
 * - @b resources (@c Array): Every item is a topic.
 *   - @b name (@c String): The name of topic
 *
 *
 * @section example
 *
 * @code
 * {
 *   "collection": "ChnWiki",
 *   "date_range" : {
 *     "start": "2011-01-01",
 *     "end":"2011-01-31"
 *   }
 * }
 * @endcode
 */
void TopicController::get_in_date_range()
{
    IZENELIB_DRIVER_BEFORE_HOOK(requireDateRange_());
    std::vector<izenelib::util::UString> topic_list;

    bool requestSent = miningSearchService_->GetTdtInTimeRange(start_date_, end_date_, topic_list);

    if (!requestSent)
    {
        response().addError(
            "Request Failed."
        );
        return;
    }
    //   std::cout<<"find similar : "<<label_list.size()<<std::endl;
    Value& resources = response()[Keys::resources];
    resources.reset<Value::ArrayType>();
    for(uint32_t i=0;i<topic_list.size();i++)
    {
        Value& new_resource = resources();
        std::string str;
        topic_list[i].convertString(str, izenelib::util::UString::UTF_8);
        new_resource[Keys::name] = str;

    }
}

/**
 * @brief Action @b get_temporal_similar. Get temporal similar topics from topic's text
 *
 * @section request
 *
 * - @b collection* (@c String): Find similar topics in this collection.
 * - @b date_range* (@c Object): Specify the date range
 *   - @c "start": (@c String): start date string
 *   - @c "end": (@c String): end date string
 * - @b topic* (@c String): The topic text
 *
 * @section response
 *
 * - @b resources (@c Object): All returned items
 *   - @b topic (@c String): The name of requested topic
 *   - @b ts (@c Array): The time series value(frequency)
 *     - @b date (@c String): the date
 *     - @b freq (@c Uint): the frequency
 *   - @b similar (@c Array): All temporal similar topics text
 *     - @b name (@c String): The name of topic
 *
 *
 * @section example
 *
 * @code
 * {
 *   "collection": "ChnWiki",
 *   "date_range" : {
 *     "start": "2011-01-01",
 *     "end":"2011-01-31"
 *   },
 *   "topic" : "abc"
 * }
 * @endcode
 */
void TopicController::get_temporal_similar()
{
    IZENELIB_DRIVER_BEFORE_HOOK(requireDateRange_());
    IZENELIB_DRIVER_BEFORE_HOOK(requireTopicText_());
    idmlib::tdt::TopicInfoType topic_info;
    bool requestSent = miningSearchService_->GetTdtTopicInfo(topic_text_, topic_info);

    if (!requestSent)
    {
        response().addError(
            "Request Failed."
        );
        return;
    }
    idmlib::tdt::TrackResult& tr = topic_info.first;
    std::cout<<"original tr size "<<tr.ts.size()<<std::endl;
    tr.LimitTsInRange(start_, end_);
    std::cout<<"new tr size "<<tr.ts.size()<<std::endl;
    //   std::cout<<"find similar : "<<label_list.size()<<std::endl;
    std::string text;
    tr.text.convertString(text, izenelib::util::UString::UTF_8);
    Value& resources = response()[Keys::resources];
    resources[Keys::topic] = text;
    Value& ts = resources[Keys::ts];
    ts.reset<Value::ArrayType>();
    for(uint32_t i=0;i<tr.ts.size();i++)
    {
        boost::gregorian::date date = tr.GetTimeByShift(i);
        std::string date_str = boost::gregorian::to_iso_extended_string(date);
        Value& new_ts_item = ts();
        new_ts_item[Keys::date] = date_str;
        new_ts_item[Keys::freq] = tr.ts[i];
    }

    const std::vector<izenelib::util::UString>& similar_topics = topic_info.second;
    Value& similar = resources[Keys::similar];
    similar.reset<Value::ArrayType>();
    for(uint32_t i=0;i<similar_topics.size();i++)
    {
        Value& new_similar = similar();
        std::string str;
        similar_topics[i].convertString(str, izenelib::util::UString::UTF_8);
        new_similar[Keys::name] = str;
    }
}

/**
 * @brief Action @b get_topics. Get topics from given content
 *
 * @section request
 *
 * - @b collection* (@c String): Find topics in this collection.
 *
 * @section response
 *
 * - @b resources (@c Object): All returned items
 * - @b content (@c String): The name of requested topic
 *
 * @section example
 *
 * @code
 * {
 *   "collection": "ChnWiki",
 *   "content" : "abc"
 * }
 * @endcode
 */
void TopicController::get_topics()
{
    IZENELIB_DRIVER_BEFORE_HOOK(requireContent_());
    size_t limit = 0;
    if(!izenelib::driver::nullValue( request()[Keys::limit] ) )
    {
        limit = asInt(request()[Keys::limit]);
    }
	
    std::vector<std::string> topic_list;
    bool requestSent = miningSearchService_->GetTopics(content_, topic_list, limit);

    if (!requestSent)
    {
        response().addError(
            "Request Failed."
        );
        return;
    }
    Value& resources = response()[Keys::resources];
    resources.reset<Value::ArrayType>();
    for(uint32_t i=0;i<topic_list.size();i++)
    {
        Value& new_resource = resources();
        new_resource[Keys::topic] = topic_list[i];
    }
}
bool TopicController::requireTID_()
{
  if(!izenelib::driver::nullValue( request()[Keys::similar_to] ) )
  {
    Value& similar_to = request()[Keys::similar_to];
    tid_ = asUint(similar_to[Keys::id]);
  }
  else
  {
      response().addError("Require field similar_to in request.");
      return false;
  }

  return true;
}

bool TopicController::requireDateRange_()
{
    if(!izenelib::driver::nullValue( request()[Keys::date_range] ) )
    {
        Value& date_range = request()[Keys::date_range];
        start_date_ = izenelib::util::UString(asString(date_range[Keys::start]), izenelib::util::UString::UTF_8);
        end_date_ = izenelib::util::UString(asString(date_range[Keys::end]), izenelib::util::UString::UTF_8);
    }
    else
    {
        response().addError("Require field date_range in request.");
        return false;
    }

    if(!idmlib::util::TimeUtil::GetDateByUString(start_date_, start_))
    {
        response().addError("start date not valid.");
        return false;
    }
    if(!idmlib::util::TimeUtil::GetDateByUString(end_date_, end_))
    {
        response().addError("end date not valid.");
        return false;
    }
    if(start_>end_)
    {
        response().addError("date_range not valid.");
        return false;
    }

    return true;
}

bool TopicController::requireTopicText_()
{
    std::string str = asString(request()[Keys::topic]);
    if (str.empty())
    {
        response().addError("Require field topic in request.");
        return false;
    }
    topic_text_ = izenelib::util::UString(str, izenelib::util::UString::UTF_8);

    return true;
}

bool TopicController::requireContent_()
{
    content_= asString(request()[Keys::content]);
    if (content_.empty())
    {
        response().addError("Require field content in request.");
        return false;
    }
    return true;
}


}
