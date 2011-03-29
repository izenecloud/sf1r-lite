#include "TaxonomyRep.h"
#include <boost/foreach.hpp>
using namespace sf1r;

TaxonomyRep::TaxonomyRep():result_(), error_("") {
    
}

TaxonomyRep::~TaxonomyRep() {
    
}

void TaxonomyRep::fill(KeywordSearchResult& searchResult)
{
    searchResult.taxonomyString_.clear();
    searchResult.numOfTGDocs_.clear();
    searchResult.tgDocIdList_.clear();
    searchResult.taxonomyLevel_.clear();
    if(error_ != "")
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
    searchResult.taxonomyString_.push_back(cluster->name_);
    searchResult.numOfTGDocs_.push_back( (cluster->docContain_).size() );
    searchResult.tgDocIdList_.push_back( cluster->docContain_ );
    searchResult.taxonomyLevel_.push_back(level);
    if( (cluster->children_).size()>0)
    {
        BOOST_FOREACH(boost::shared_ptr<TgClusterRep>& _cluster, cluster->children_)
        {
            fill_(searchResult,_cluster,level+1);
        }
    }
}
