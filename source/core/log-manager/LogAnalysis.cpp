#include "LogManager.h"
#include "UserQuery.h"
#include "LogAnalysis.h"
#include <query-manager/QMCommonFunc.h>
//extern class QueryType;

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

//author wang qian
void LogAnalysis::getRecentKeywordFreqList(const std::string& collectionName, uint32_t limit, std::vector<QueryType>& queryList)
{
    queryList.resize(0);
    if (limit==0 ) return;
    std::vector<UserQuery> query_records;
    UserQuery::find(
        "query ,max(hit_docs_num) as hit_docs_num,count(*) as count",
        "collection = '" + collectionName + "'" + " and " + "hit_docs_num > 0",
        "query",
        "",
        boost::lexical_cast<std::string>(limit),
        query_records);

    std::vector<UserQuery>::const_iterator it = query_records.begin();
    for ( ; it!=query_records.end(); ++it )
    {
        QueryType TempQuery;
        TempQuery.strQuery_ = it->getQuery();
        TempQuery.freq_ = it->getCount();
        //TempQuery.HitNum_ = it->gethit_docs_num();
        queryList.push_back(TempQuery );
    }
}

//



}
