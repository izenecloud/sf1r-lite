#include "PropertyLabel.h"
#include <boost/lexical_cast.hpp>

using namespace std;
using namespace boost;
using namespace boost::posix_time;

namespace sf1r {

const char* PropertyLabel::ColumnName[EoC] = { "collection", "label_id", "label_name", "hit_docs_num" };

const char* PropertyLabel::ColumnMeta[EoC] = { "Text", "integer", "Text", "integer" };

const char* PropertyLabel::TableName = "property_labels";

void PropertyLabel::save( std::map<std::string, std::string> & rawdata ) {
    rawdata.clear();
    if(hasCollection() )
        rawdata[ ColumnName[Collection] ] = getCollection();
    if(hasLabelId() )
        rawdata[ ColumnName[LabelId] ] = getLabelId();
    if(hasLabelName() )
        rawdata[ ColumnName[LabelName] ] = getLabelName();
    if(hasHitDocsNum() )
        rawdata[ ColumnName[HitDocsNum] ] = boost::lexical_cast<string>(getHitDocsNum());
}

void PropertyLabel::load( const std::map<std::string, std::string> & rawdata ) {
    for( map<string,string>::const_iterator it = rawdata.begin(); it != rawdata.end(); it++ ) {
        if(it->first == ColumnName[Collection] ) {
            setCollection(it->second);
        } else if(it->first == ColumnName[LabelId]) {
            setLabelId(boost::lexical_cast<std::size_t>(it->second));
        }else if(it->first == ColumnName[LabelName]) {
            setLabelName(it->second);
        } else if(it->first == ColumnName[HitDocsNum]) {
            setHitDocsNum(boost::lexical_cast<std::size_t>(it->second));
        }
    }
}

}
