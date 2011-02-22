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
        std::string query_sql = "select DISTINCT query from user_queries where collection='"+collectionName+"'  order by TimeStamp desc limit "+boost::lexical_cast<std::string>(limit);
        std::list<std::map<std::string, std::string> > query_records;
        UserQuery::find_by_sql(query_sql, query_records);
        std::list<std::map<std::string, std::string> >::iterator it = query_records.begin();
        for( ;it!=query_records.end();++it )
        {
            izenelib::util::UString uquery( (*it)["query"], izenelib::util::UString::UTF_8);
            if( QueryUtility::isRestrictWord( uquery ) )
            {
                continue;
            }
            recentKeywords.push_back( uquery );
        }
    }

}
