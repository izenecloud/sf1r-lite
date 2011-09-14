#include "LogAnalysisController.h"

#include <log-manager/SystemEvent.h>
#include <log-manager/UserQuery.h>
#include <log-manager/ProductInfo.h>
#include <common/SFLogger.h>
#include <common/parsers/OrderArrayParser.h>
#include <common/parsers/ConditionArrayParser.h>
#include <common/parsers/SelectArrayParser.h>

#include <list>
#include <vector>

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/any.hpp>

using namespace std;
using namespace boost;
using namespace boost::posix_time;

namespace sf1r
{

using driver::Keys;
using namespace izenelib::driver;

std::string LogAnalysisController::parseSelect(bool & isExistAggregateFunc)
{
    vector<string> results;
    SelectArrayParser selectArrayParser;
    if (!nullValue(request()[Keys::select]))
    {
        selectArrayParser.parse(request()[Keys::select]);
        for ( vector<SelectFieldParser>::const_iterator it = selectArrayParser.parsedSelect().begin();
                it != selectArrayParser.parsedSelect().end(); it++ )
        {
            if (!(it->func()).empty())
            {
                isExistAggregateFunc = true;
                 results.push_back( str_concat(it->func(), "(", it->property(), ") as count") );
            }
            else
            {
                 results.push_back(it->property());
            }
        }
    }
    return str_join(results, ",");
}

std::string LogAnalysisController::parseOrder()
{
    vector<string> results;
    OrderArrayParser orderArrayParser;
    if (!nullValue(request()[Keys::sort]))
    {
        orderArrayParser.parse(request()[Keys::sort]);
        for ( vector<OrderParser>::const_iterator it = orderArrayParser.parsedOrders().begin();
                it != orderArrayParser.parsedOrders().end(); it++ )
        {
            results.push_back( str_concat(it->property(), " ", it->ascendant() ? "ASC" : "DESC") );
        }
    }
    return str_join(results, ",");
}

std::string LogAnalysisController::parseConditions()
{
    vector<string> results;
    ConditionArrayParser conditonArrayParser;
    if (!nullValue(request()[Keys::conditions]))
    {
        conditonArrayParser.parse(request()[Keys::conditions]);
        for ( vector<ConditionParser>::const_iterator it = conditonArrayParser.parsedConditions().begin();
                it != conditonArrayParser.parsedConditions().end(); it++ )
        {
            if ( (it->op() == "=" || it->op() == "<"  || it->op() == ">" ||
                    it->op() == "<>" || it->op() == ">=" || it->op() == "<=") && it->size() == 1 )
            {
                results.push_back( str_concat(it->property(), it->op(), to_str((*it)(0))) );
            }
            else if (it->op() == "between" && it->size() == 2 )
            {
                stringstream ss;
                ss << it->property() << " between " << to_str((*it)(0)) << " and " << to_str((*it)(1));
                results.push_back( ss.str() );
            }
            else if (it->op() == "in" && it->size() > 0 )
            {
                stringstream ss;
                ss << it->property() << " in(";
                for (size_t i=0; i<it->size()-1; i++)
                {
                    ss << to_str((*it)(i)) << ",";
                }
                ss <<  to_str((*it)(it->size()-1)) << ")";
                results.push_back(ss.str());
            }
        }
    }
    return str_join(results, " and ");
}

std::string LogAnalysisController::parseGroupBy()
{
    vector<string> results;
    if (!nullValue(request()[Keys::groupby]))
    {
            const Value::ArrayType* array = request()[Keys::groupby].getPtr<Value::ArrayType>();
            if (array)
            {
                for (std::size_t i = 0; i < array->size(); ++i)
                {
                    std::string column = asString((*array)[i]);
                    boost::to_lower(column);
                    results.push_back(column);
                }
            }
     }
     return str_join(results, ",");
}

/**
 * @brief Action \b system_events.
 *
 * @section request
 *
 * - @b select (@c Array): Select columns in result. OrderArrayParser.
 * - @b conditions (@c Array): Result filtering conditions. See ConditionArrayParser.
 * - @b sort (@c Array): Sort result. See OrderArrayParser.
 *
 * @section response
 *
 * - @b system_events All system events which fit conditions.
 *
 * @section example
 *
 * Request
 * @code
 * {
 *   "select"=>["source", "content", "timestamp"],
 *   "conditions"=>[
        {"property":"timestamp", "operator":"range", "value":[1.0, 10.0]},
        {"property":"level", "operator":"in", "value":["warn", "error"]}
     ],
     "sort"=>["timestamp"]
 * }
 * @endcode
 *
 * response
 * @code
 * {
 *   "system_events" => [
 *      {"level"=>"error", "source"=>"SYS", "content"=>"Out of memory", "timestamp"=>"2010-Jul-16 18:11:38" }
 *   ]
 * }
 * @endcode
 *
 */
void LogAnalysisController::system_events()
{
    bool isExistAggregateFunc = false;
    string select = parseSelect(isExistAggregateFunc);
    string conditions = parseConditions();
    string group = parseGroupBy();
    string order = parseOrder();
    string limit = asString(request()[Keys::limit]);
    vector<SystemEvent> results;

    if ( !SystemEvent::find(select, conditions, group, order, limit, results) )
    {
        response().addError("[LogManager] Fail to process such a request");
        return;
    }

    Value& systemEvents = response()[Keys::system_events];
    systemEvents.reset<Value::ArrayType>();
    for (vector<SystemEvent>::iterator it = results.begin(); it != results.end(); it ++ )
    {
        Value& systemEvent = systemEvents();
        if (it->hasLevel()) systemEvent[Keys::level] = it->getLevel();
        if (it->hasSource()) systemEvent[Keys::source] = it->getSource();
        if (it->hasContent()) systemEvent[Keys::content] = it->getContent();
        if (it->hasTimeStamp()) systemEvent[Keys::timestamp] = to_simple_string(it->getTimeStamp());
    }
}

/**
 * @brief Action \b user_queries.
 *
 * @section request
 *
 * - @b select (@c Array): Select columns in result. OrderArrayParser.
 * - @b conditions (@c Array): Result filtering conditions. See ConditionArrayParser.
 * - @b sort (@c Array): Sort result. See OrderArrayParser.
 *
 * @section response
 *
 * - @b user_queries All system events which fit conditions.
 *
 */
void LogAnalysisController::user_queries()
{
    bool isExistAggregateFunc = false;
    string select = parseSelect(isExistAggregateFunc);
    string conditions = parseConditions();
    string group = parseGroupBy();
    string order = parseOrder();
    string limit = asString(request()[Keys::limit]);
    vector<UserQuery> results;
    std::list< std::map<std::string, std::string> > sqlResults;

    if ( !UserQuery::find(select, conditions, group, order, limit, results) )
    {
        response().addError("[LogManager] Fail to process such a request");
        return;
    }

    if (isExistAggregateFunc)
    {
        std::stringstream sql;

        sql << "select " << (select.size() ? select : "*");
        sql << " from " << UserQuery::TableName ;
        if ( conditions.size() )
        {
             sql << " where " << conditions;
        }
        if ( group.size() )
        {
             sql << " group by " << group;
        }
        if ( order.size() )
        {
             sql << " order by " << order;
        }
        if ( limit.size() )
        {
            sql << " limit " << limit;
        }
        sql << ";";
        UserQuery::find_by_sql(sql.str(), sqlResults);
    }

    Value& userQueries = response()[Keys::user_queries];
    userQueries.reset<Value::ArrayType>();
    std::size_t i = 0;
    if (isExistAggregateFunc)
    {
        for (std::list< std::map<std::string, std::string> >::iterator it = sqlResults.begin();
                it!=sqlResults.end() && i < results.size(); it++, i++)
        {
            Value& userQuery = userQueries();
            if (results[i].hasQuery()) userQuery[Keys::query] = results[i].getQuery();
            if (results[i].hasCollection()) userQuery[Keys::collection] = results[i].getCollection();
            if (results[i].hasHitDocsNum()) userQuery[Keys::hit_docs_num] = results[i].getHitDocsNum();
            if (results[i].hasPageStart()) userQuery[Keys::page_start] = results[i].getPageStart();
            if (results[i].hasPageCount()) userQuery[Keys::page_count] = results[i].getPageCount();
            if (results[i].hasSessionId()) userQuery[Keys::session_id] = results[i].getSessionId();
            if (results[i].hasDuration()) userQuery[Keys::duration] = to_simple_string(results[i].getDuration());
            if (results[i].hasTimeStamp()) userQuery[Keys::timestamp] = to_simple_string(results[i].getTimeStamp());
            userQuery[Keys::count] = (*it)["count"];
        }
    }
    else
    {
        for (std::vector<UserQuery>::iterator it = results.begin(); it != results.end(); it++)
        {
            Value& userQuery = userQueries();
            if (it->hasQuery()) userQuery[Keys::query] = it->getQuery();
            if (it->hasCollection()) userQuery[Keys::collection] = it->getCollection();
            if (it->hasHitDocsNum()) userQuery[Keys::hit_docs_num] = it->getHitDocsNum();
            if (it->hasPageStart()) userQuery[Keys::page_start] = it->getPageStart();
            if (it->hasPageCount()) userQuery[Keys::page_count] = it->getPageCount();
            if (it->hasSessionId()) userQuery[Keys::session_id] = it->getSessionId();
            if (it->hasDuration()) userQuery[Keys::duration] = to_simple_string(it->getDuration());
            if (it->hasTimeStamp()) userQuery[Keys::timestamp] = to_simple_string(it->getTimeStamp());
        }
    }
}

/**
 * @brief Action \b merchant_count.
 *
 * @section request
 *
 * - @b conditions (@c Array): Result filtering conditions. See ConditionArrayParser.
 *
 * @section response
 *
 * - @b merchant_count The merchant count in all the records which fit the conditions.
 *
 * @section example
 *
 * Request
 * @code
 * {
 *   "conditions"=>[
        {"property":"collection", "operator":"=", "value":"intel"}
     ]
 * }
 * @endcode
 *
 * response
 * @code
 * {
 *   "merchant_count" => {
 *      "count"=>"100"
 *   }
 * }
 * @endcode
 *
 */
void LogAnalysisController::merchant_count()
{
    string select = "count(distinct Source) as count";
    string conditions = parseConditions();

    std::list< std::map<std::string, std::string> > sqlResults;
    std::stringstream sql;

    sql << "select " << select;
    sql << " from " << ProductInfo::TableName;
    if ( conditions.size() )
    {
         sql << " where " << conditions;
    }
    sql << ";";
    std::cerr << sql.str() << std::endl;
    ProductInfo::find_by_sql(sql.str(), sqlResults);

    Value& productInfo = response()[Keys::merchant_count];
    std::list< std::map<std::string, std::string> >::iterator it = sqlResults.begin();
    productInfo[Keys::count] = (*it)["count"];
}


/**
 * @brief Action \b product_update_info.
 *
 * @section request
 *
 * - @b conditions (@c Array): Result filtering conditions. See ConditionArrayParser.
 *
 * @section response
 *
 * - @b product_update_info The product updated information for records with specified source which fit conditions.
 *
 * @section example
 *
 * Request
 * @code
 * {
 *   "conditions"=>[
        {"property":"source", "operator":"=", "value":"当当网"},
        {"property":"collection", "operator":"=", "value":"intel"},
        {"property":"timestamp", "operator":"range", "value":[1.0, 10.0]},
     ]
 * }
 * @endcode
 *
 * response
 * @code
 * {
 *   "product_update_info" => {
 *      "count"=>"1000", "update_info"=>"50", "delete_info"=>"20"
 *   }
 * }
 * @endcode
 *
 */
void LogAnalysisController::product_update_info()
{
    string select = "sum(Num) as sum";
    string conditions = parseConditions() + " and ";
    conditions += "Flag = " + str_concat("'", "insert", "'");

    ///Get index result
    std::list< std::map<std::string, std::string> > insertResults;
    std::stringstream sql;

    sql << "select " << select;
    sql << " from " << ProductInfo::TableName;
    if( conditions.size() )
    {
        sql << " where " << conditions;
    }
    sql << ";";
    std::cerr << sql.str() << std::endl;
    ProductInfo::find_by_sql(sql.str(), insertResults);

    ///Get updated result
    conditions = parseConditions() + " and ";
    conditions += "Flag = " + str_concat("'", "update", "'");

    std::list< std::map<std::string, std::string> > updateResults;
    sql.clear();
    sql.str("");

    sql << "select " << select;
    sql << " from " << ProductInfo::TableName;
    if( conditions.size() )
    {
        sql << " where " << conditions;
    }
    sql <<";";
    std::cerr<< sql.str() << std::endl;
    ProductInfo::find_by_sql(sql.str(), updateResults);

    ///Get deleted result
    conditions = parseConditions() + " and ";
    conditions += "Flag = " + str_concat("'", "delete", "'");

    std::list< std::map<std::string, std::string> > deleteResults;
    sql.clear();
    sql.str("");

    sql<<"select " << select;
    sql<<" from "<< ProductInfo::TableName;
    if( conditions.size() )
    {
        sql<<" where " << conditions;
    }
    sql<<";";
    std::cerr<< sql.str() <<std::endl;
    ProductInfo::find_by_sql(sql.str(), deleteResults);

    ///Output
    Value& productInfo = response()[Keys::product_update_info];
    std::list< std::map<std::string, std::string> >::iterator iter = insertResults.begin();
    std::map<std::string, std::string>::const_iterator map_iter;

    productInfo[Keys::count] = (*iter)["sum"];
    if( updateResults.size() && (map_iter = updateResults.front().find("sum")) != updateResults.front().end())
    {
        productInfo[Keys::update_info] = map_iter->second;
    }
    if( deleteResults.size() && (map_iter = deleteResults.front().find("sum")) != deleteResults.front().end())
    {
        productInfo[Keys::delete_info] = map_iter->second;
    }
}

/**
 * @brief Action \b inject_user_queries.
 *
 * @section request
 *
 * The request format is identical with the response of \b user_queries.
 *
 * - @b user_queries (@c Array): Array of queries with following fields
 *   - @b query* (@c String): keywords.
 *   - @b collection* (@c String): collection name.
 *   - @b hit_docs_num (@c Uint = 0): Number of hit documents.
 *   - @b page_start (@c Uint = 0): Page start index offset.
 *   - @b page_count (@c Uint = 0): Request number of documents in page.
 *   - @b duration (@c String = 0): Duration in format HH:MM:SS.fffffffff, where
 *        fffffffff is fractional seconds and can be omit.
 *   - @b timestamp (@c String = now): Time in format YYYY-mm-dd
 *     HH:MM:SS.fffffffff. If this is not specified, current time is used.
 * - @b conditions (@c Array): Result filtering conditions. See ConditionArrayParser.
 * - @b sort (@c Array): Sort result. See OrderArrayParser.
 *
 * @section response
 *
 * - @b user_queries All system events which fit conditions.
 *
 */
void LogAnalysisController::inject_user_queries()
{
    static const std::string session = "SESSION NOT USED";

    if (nullValue(request()[Keys::user_queries]))
    {
        return;
    }

    Value::ArrayType* queries = request()[Keys::user_queries].getArrayPtr();
    if (!queries)
    {
        response().addError(Keys::user_queries + " must be an array");
        return;
    }

    for (std::size_t i = 0; i < queries->size(); ++i)
    {
        Value& query = (*queries)[i];

        std::string keywords = asString(query[Keys::query]);
        std::string collection = asString(query[Keys::collection]);

        if (keywords.empty() || collection.empty())
        {
            response().addWarning("Require fields query and collection");
        }
        else
        {
            UserQuery queryLog;
            queryLog.setQuery(keywords);
            queryLog.setCollection(collection);
            queryLog.setHitDocsNum(asUint(query[Keys::hit_docs_num]));
            queryLog.setPageStart(asUint(query[Keys::page_start]));
            queryLog.setPageCount(asUint(query[Keys::page_count]));
            queryLog.setSessionId(session);

            // duration
            queryLog.setDuration(boost::posix_time::time_duration());
            try
            {
                if (query.hasKey(Keys::duration))
                {
                    queryLog.setDuration(
                        boost::posix_time::duration_from_string(
                            asString(query[Keys::duration])
                        )
                    );
                }
            }
            catch (const std::exception& e)
            {
                response().addWarning(
                    "Invalid duration: " + asString(query[Keys::duration])
                );
            }

            // timestamp
            // default is now
            queryLog.setTimeStamp(boost::posix_time::second_clock::local_time());
            try
            {
                if (query.hasKey(Keys::timestamp))
                {
                    queryLog.setTimeStamp(
                        boost::posix_time::time_from_string(
                            asString(query[Keys::timestamp])
                        )
                    );
                }
            }
            catch (const std::exception& e)
            {
                response().addWarning(
                    "Invalid timestamp: " + asString(query[Keys::timestamp])
                );
            }

            queryLog.save();
        }
    }
}

/**
 * @brief Action \b delete_record_from_system_events.
 *
 * @section request
 *
 * - @b conditions (@c Array): Record deleted conditions. See driver::ConditionArrayParser.
 *
 * @section response
 *
 * - @b No extra fields.
 *
 * @section example
 *
 * Request
 * @code
 * {
 *   "conditions"=>[
        {"property":"timestamp", "operator":"range", "value":[1.0, 10.0]},
        {"property":"level", "operator":"in", "value":["warn", "error"]}
     ]
 * }
 * @endcode
 *
 */
void LogAnalysisController::delete_record_from_system_events()
{
    string conditions = parseConditions();

    if ( !SystemEvent::del_record(conditions) )
    {
        response().addError("[LogManager] Fail to process such a request");
        return;
    }
}

/**
 * @brief Action \b delete_record_from_user_queries.
 *
 * @section request
 *
 * - @b conditions (@c Array): Record deleted conditions. See driver::ConditionArrayParser.
 *
 * @section response
 *
 * - @b No extra fields.
 *
 * @section example
 *
 * Request
 * @code
 * {
 *   "conditions"=>[
        {"property":"timestamp", "operator":"range", "value":[1.0, 10.0]},
        {"property":"query", "operator":"in", "value":["中国", "人民"]}
     ]
 * }
 * @endcode
 *
 */
void LogAnalysisController::delete_record_from_user_queries()
{
    string conditions = parseConditions();

    if ( !UserQuery::del_record(conditions) )
    {
        response().addError("[LogManager] Fail to process such a request");
        return;
    }
}

void LogAnalysisController::delete_database()
{
    if (!sflog->del_database())
    {
        response().addError("[LogManager] Fail to process such a request");
        return;
    }
}

} // namespace sf1r
