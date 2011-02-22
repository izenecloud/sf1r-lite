#include "SystemEvent.h"

using namespace std;
using namespace boost::posix_time;

namespace sf1r {

const char* SystemEvent::ColumnName[EoC] = { "level", "Source", "Content", "TimeStamp" };

const char* SystemEvent::ColumnMeta[EoC] = { "Text", "TEXT", "TEXT", "TEXT" };

const char* SystemEvent::TableName = "system_events";

void SystemEvent::save( std::map<std::string, std::string> & rawdata ) {
    rawdata.clear();
    if(hasLevel() )
        rawdata[ ColumnName[Level] ] = getLevel();
    if(hasSource() )
        rawdata[ ColumnName[Source] ] = getSource();
    if(hasContent() )
        rawdata[ ColumnName[Content] ] = getContent();
    if(hasTimeStamp() )
        rawdata[ ColumnName[TimeStamp] ] = to_iso_string(getTimeStamp());
}

void SystemEvent::load( const std::map<std::string, std::string> & rawdata ) {
    for( map<string,string>::const_iterator it = rawdata.begin(); it != rawdata.end(); it++ ) {
        if(it->first == ColumnName[Level] ) {
            setLevel(it->second);
        } else if(it->first == ColumnName[Source]) {
            setSource(it->second);
        } else if(it->first == ColumnName[Content]) {
            setContent(it->second);
        } else if(it->first == ColumnName[TimeStamp]) {
            setTimeStamp(from_iso_string(it->second));
        }
    }
}

}
