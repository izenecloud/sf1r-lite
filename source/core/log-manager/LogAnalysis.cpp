#include "LogManager.h"
#include "UserQuery.h"
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
        "DISTINCT query",
        "collection = '" + collectionName + "'",
        "",
        "TimeStamp desc",
        boost::lexical_cast<std::string>(limit),
        query_records);

    std::vector<UserQuery>::const_iterator it = query_records.begin();
    for ( ;it!=query_records.end();++it )
    {
        izenelib::util::UString uquery(it->getQuery(), izenelib::util::UString::UTF_8);
        if ( QueryUtility::isRestrictWord( uquery ) )
        {
            continue;
        }
        recentKeywords.push_back( uquery );
    }
}

}
