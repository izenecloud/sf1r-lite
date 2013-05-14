#include "LogManager.h"
#include "LogAnalysis.h"
#include <query-manager/QMCommonFunc.h>

namespace sf1r
{

void LogAnalysis::getRecentKeywordList(const std::string& collectionName, uint32_t limit, std::vector<izenelib::util::UString>& recentKeywords)
{
    recentKeywords.resize(0);
    if (limit==0 ) return;
    std::vector<UserQuery> query_records;
    UserQuery::find(
        "distinct query",
        "collection = '" + collectionName + "'" + " and " + "hit_docs_num > 0",
        "",
        "TimeStamp desc",
        boost::lexical_cast<std::string>(limit),
        query_records);

    std::vector<UserQuery>::const_iterator it = query_records.begin();
    for ( ; it!=query_records.end(); ++it )
    {
        izenelib::util::UString uquery(it->getQuery(), izenelib::util::UString::UTF_8);
        if ( QueryUtility::isRestrictWord( uquery ) )
        {
            continue;
        }
        recentKeywords.push_back( uquery );
    }
}

//stime_string: 120days, 90days, 7days, 1days
void LogAnalysis::getRecentKeywordFreqList(const std::string& collectionName, const std::string& time_string, std::vector<UserQuery>& queryList)
{
    UserQuery::find(
        "query ,max(hit_docs_num) as hit_docs_num,count(*) as count",
        "collection = '" + collectionName + "' and hit_docs_num > 0 and TimeStamp >= '" + time_string +"'",
        "query",
        "",
        "",
        queryList);
}
}
