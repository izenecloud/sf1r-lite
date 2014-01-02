#include "TaxonomyRep.h"
#include <boost/foreach.hpp>
using namespace sf1r;

TaxonomyRep::TaxonomyRep():result_(), error_("")
{

}

TaxonomyRep::~TaxonomyRep()
{

}

void TaxonomyRep::fill(KeywordSearchResult& searchResult)
{
    searchResult.tg_info_.taxonomyString_.clear();
    searchResult.tg_info_.numOfTGDocs_.clear();
    searchResult.tg_info_.tgDocIdList_.clear();
    searchResult.tg_info_.taxonomyLevel_.clear();
    if (error_ != "")
    {
        searchResult.error_ += "[TG:"+error_+"]";
    }
    BOOST_FOREACH(boost::shared_ptr<TgClusterRep>& _cluster, result_)
    {
        fill_(searchResult,_cluster,0);
    }
}

void TaxonomyRep::fill_(KeywordSearchResult& searchResult, boost::shared_ptr<TgClusterRep> cluster, uint8_t level)
{
    searchResult.tg_info_.taxonomyString_.push_back(ustr_to_propstr(cluster->name_));
    searchResult.tg_info_.numOfTGDocs_.push_back( (cluster->docContain_).size() );
    searchResult.tg_info_.tgDocIdList_.push_back( cluster->docContain_ );
    searchResult.tg_info_.taxonomyLevel_.push_back(level);
    if ( (cluster->children_).size()>0)
    {
        BOOST_FOREACH(boost::shared_ptr<TgClusterRep>& _cluster, cluster->children_)
        {
            fill_(searchResult,_cluster,level+1);
        }
    }
}
