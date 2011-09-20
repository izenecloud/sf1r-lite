/**
 * \file ProductInfo.cpp
 * \brief 
 * \date Sep 9, 2011
 * \author Xin Liu
 */

#include "ProductInfo.h"
#include <boost/lexical_cast.hpp>

using namespace std;
using namespace boost;
using namespace boost::posix_time;

namespace sf1r {

const char* ProductInfo::ColumnName[EoC] = { "Source", "Collection", "Num", "Flag", "TimeStamp" };

const char* ProductInfo::ColumnMeta[EoC] = { "TEXT", "TEXT", "integer", "TEXT", "TEXT" };

const char* ProductInfo::TableName = "product_info";

void ProductInfo::save( std::map<std::string, std::string> & rawdata ) {
    rawdata.clear();
    if(hasSource() )
        rawdata[ ColumnName[Source] ] = getSource();
    if(hasCollection() )
        rawdata[ ColumnName[Collection] ] = getCollection();
    if(hasNum() )
        rawdata[ ColumnName[Num] ] = boost::lexical_cast<std::string>(getNum());
    if(hasFlag() )
        rawdata[ ColumnName[Flag] ] = getFlag();
    if(hasTimeStamp() )
        rawdata[ ColumnName[TimeStamp] ] = to_iso_string(getTimeStamp());
}

void ProductInfo::load( const std::map<std::string, std::string> & rawdata ) {
    for( map<string,string>::const_iterator it = rawdata.begin(); it != rawdata.end(); it++ ) {
        if(it->first == ColumnName[Source] ) {
            setSource(it->second);
        } else if(it->first == ColumnName[Collection]) {
            setCollection(it->second);
        } else if(it->first == ColumnName[Num]) {
            setNum(boost::lexical_cast<uint32_t>(it->second));
        } else if(it->first == ColumnName[Flag]) {
            setFlag(it->second);
        } else if(it->first == ColumnName[TimeStamp]) {
            setTimeStamp(from_iso_string(it->second));
        }
    }
}

}
