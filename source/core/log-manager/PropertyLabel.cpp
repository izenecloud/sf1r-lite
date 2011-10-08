#include "PropertyLabel.h"
#include <boost/lexical_cast.hpp>

using namespace std;
using namespace boost;
using namespace boost::posix_time;

namespace sf1r {

const char* PropertyLabel::ColumnName[EoC] = { "collection", "property_name", "label_name", "hit_docs_num", "TimeStamp" };

const char* PropertyLabel::ColumnMeta[EoC] = { "Text", "TEXT", "TEXT", "integer", "TEXT" };

const char* PropertyLabel::TableName = "property_labels";

void PropertyLabel::save( std::map<std::string, std::string> & rawdata ) {
    rawdata.clear();
    if(hasCollection() )
        rawdata[ ColumnName[Collection] ] = getCollection();
    if(hasPropertyName() )
        rawdata[ ColumnName[PropertyName] ] = getPropertyName();
    if(hasLabelName() )
        rawdata[ ColumnName[LabelName] ] = getLabelName();
    if(hasHitDocsNum() )
        rawdata[ ColumnName[HitDocsNum] ] = boost::lexical_cast<string>(getHitDocsNum());
    if(hasTimeStamp() )
        rawdata[ ColumnName[TimeStamp] ] = to_iso_string(getTimeStamp());
}

void PropertyLabel::load( const std::map<std::string, std::string> & rawdata ) {
    for( map<string,string>::const_iterator it = rawdata.begin(); it != rawdata.end(); it++ ) {
        if(it->first == ColumnName[Collection] ) {
            setCollection(it->second);
        } else if(it->first == ColumnName[PropertyName]) {
            setPropertyName(it->second);
        } else if(it->first == ColumnName[LabelName]) {
            setLabelName(it->second);
        } else if(it->first == ColumnName[HitDocsNum]) {
            setHitDocsNum(boost::lexical_cast<size_t>(it->second));
        } else if(it->first == ColumnName[TimeStamp]) {
            setTimeStamp(from_iso_string(it->second));
        }
    }
}

}
