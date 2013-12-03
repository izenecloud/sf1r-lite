#include "PropertyLabel.h"
#include <boost/lexical_cast.hpp>

using namespace std;

namespace sf1r {

const char* PropertyLabel::ColumnName[EoC] = { "collection", "label_name", "hit_docs_num" };

const char* PropertyLabel::ColumnMeta[EoC] = { "Text", "Text", "integer" };

const char* PropertyLabel::TableName = "property_labels";

const std::string PropertyLabel::service_ = "property-label";

void PropertyLabel::save( std::map<std::string, std::string> & rawdata )
{
    rawdata.clear();
    if (hasCollection())
        rawdata[ ColumnName[Collection] ] = getCollection();
    if (hasLabelName())
        rawdata[ ColumnName[LabelName] ] = getLabelName();
    if (hasHitDocsNum())
        rawdata[ ColumnName[HitDocsNum] ] = boost::lexical_cast<string>(getHitDocsNum());
}

void PropertyLabel::load( const std::map<std::string, std::string> & rawdata )
{
    for( map<string,string>::const_iterator it = rawdata.begin();
            it != rawdata.end(); it++ )
    {
        if (it->first == ColumnName[Collection]) {
            setCollection(it->second);
        } else if (it->first == ColumnName[LabelName]) {
            setLabelName(it->second);
        } else if (it->first == ColumnName[HitDocsNum]) {
            setHitDocsNum(boost::lexical_cast<std::size_t>(it->second));
        }
    }
}

void PropertyLabel::save_to_logserver()
{
    LogAnalysisConnection& conn = LogAnalysisConnection::instance();
    InsertPropLabelRequest req;
    req.param_.collection_ = collection_;
    req.param_.label_name_ = labelName_;
    req.param_.hitnum_ = hitDocsNum_;

    bool res;
    conn.syncRequest(req, res);
}

void PropertyLabel::get_from_logserver(const std::string& collection,
    std::list<std::map<std::string, std::string> >& res)
{
    LogAnalysisConnection& conn = LogAnalysisConnection::instance();
    GetPropLabelRequest req;
    req.param_.collection_ = collection;

    conn.syncRequest(req, res);
}

void PropertyLabel::del_from_logserver(const std::string& collection)
{
    LogAnalysisConnection& conn = LogAnalysisConnection::instance();
    DelPropLabelRequest req;
    req.param_.collection_ = collection;

    bool res;
    conn.syncRequest(req, res);
}

}
