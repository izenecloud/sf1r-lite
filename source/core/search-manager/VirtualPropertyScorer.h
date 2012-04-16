#ifndef CORE_SEARCH_MANAGER_VIRTUAL_PROPERTY_SCORER_H
#define CORE_SEARCH_MANAGER_VIRTUAL_PROPERTY_SCORER_H

#include "DocumentIterator.h"

namespace sf1r{

class VirtualPropertyScorer : public DocumentIterator
{
public:
    VirtualPropertyScorer(
        const property_weight_map& propertyWeightMap,
        const std::vector<unsigned int>& propertyIds);

    virtual ~VirtualPropertyScorer();

    void add(DocumentIterator* pDocIterator) {
        pIter_ = pDocIterator;
    }

    bool next() {
        return pIter_->next();
    }

    docid_t doc() {
        return pIter_->doc();
    }

    void doc_item(RankDocumentProperty& rankDocumentProperty, 
                        unsigned propIndex = 0) {}

    void df_cmtf(DocumentFrequencyInProperties& dfmap,
                        CollectionTermFrequencyInProperties& ctfmap,
                        MaxTermFrequencyInProperties& maxtfmap) {
        pIter_->df_cmtf(dfmap,ctfmap,maxtfmap);
    }

    count_t tf() { return 0;}

    docid_t skipTo(docid_t target) {
        return pIter_->skipTo(target);
    }

    double score(
        const RankQueryProperty& rankQueryProperty,
        const boost::shared_ptr<PropertyRanker>& propertyRanker
    );

protected:
    void init_(const property_weight_map& propertyWeightMap);
	
    DocumentIterator* pIter_;
    std::vector<propertyid_t> indexPropertyIdList_;
    std::vector<double> propertyWeightList_;
    unsigned numProperties_;
	
    boost::mutex mutex_;

    ///@brief reuse in score() for performance, so score() is not thread-safe
    RankDocumentProperty rankDocumentProperty_;

    std::vector<RankQueryProperty> rankQueryProperties_;
    std::vector<boost::shared_ptr<PropertyRanker> > propertyRankers_;
    friend class QueryBuilder;
};

}

#endif

