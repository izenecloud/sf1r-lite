#include "UserQuery.h"
#include <boost/lexical_cast.hpp>

using namespace std;
using namespace boost;
using namespace boost::posix_time;

namespace sf1r
{

const char* UserQuery::ColumnName[EoC] = { "query", "collection", "hit_docs_num", \
        "page_start", "page_count", "session_id", "duration" , "TimeStamp"
                                         };

const char* UserQuery::ColumnMeta[EoC] = { "Text", "TEXT", "integer", "integer", "integer", "TEXT", "CHAR(15)", "CHAR(22)" };

const char* UserQuery::TableName = "user_queries";

void UserQuery::save( std::map<std::string, std::string> & rawdata )
{
    rawdata.clear();
    if (hasQuery() )
        rawdata[ ColumnName[Query] ] = getQuery();
    if (hasCollection() )
        rawdata[ ColumnName[Collection] ] = getCollection();
    if (hasHitDocsNum() )
        rawdata[ ColumnName[HitDocsNum] ] = boost::lexical_cast<string>(getHitDocsNum());
    if (hasPageStart() )
        rawdata[ ColumnName[PageStart] ] = boost::lexical_cast<string>(getPageStart());
    if (hasPageCount() )
        rawdata[ ColumnName[PageCount] ] = boost::lexical_cast<string>(getPageCount());
    if (hasSessionId() )
        rawdata[ ColumnName[SessionId] ] = getSessionId();
    if (hasDuration() )
        rawdata[ ColumnName[Duration] ] = to_simple_string(getDuration());
    if (hasTimeStamp() )
        rawdata[ ColumnName[TimeStamp] ] = to_iso_string(getTimeStamp());
}

void UserQuery::load( const std::map<std::string, std::string> & rawdata )
{
    for ( map<string,string>::const_iterator it = rawdata.begin(); it != rawdata.end(); it++ )
    {
        if (it->first == ColumnName[Query] )
        {
            setQuery(it->second);
        }
        else if (it->first == ColumnName[Collection])
        {
            setCollection(it->second);
        }
        else if (it->first == ColumnName[HitDocsNum])
        {
            setHitDocsNum(boost::lexical_cast<size_t>(it->second));
        }
        else if (it->first == ColumnName[PageStart])
        {
            setPageStart(boost::lexical_cast<uint32_t>(it->second));
        }
        else if (it->first == ColumnName[PageCount])
        {
            setPageCount(boost::lexical_cast<uint32_t>(it->second));
        }
        else if (it->first == ColumnName[SessionId])
        {
            setSessionId(it->second);
        }
        else if (it->first == ColumnName[Duration])
        {
            setDuration(duration_from_string(it->second));
        }
        else if (it->first == ColumnName[TimeStamp])
        {
            setTimeStamp(from_iso_string(it->second));
        }
    }
}

}
