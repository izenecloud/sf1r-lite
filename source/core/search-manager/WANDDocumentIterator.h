/**
 * \file WANDDocumentIterator.h
 * \brief
 * \date Feb 29, 2012
 * \author Xin Liu
 */

#ifndef _WAND_DOCUMENT_ITERATOR_H_
#define _WAND_DOCUMENT_ITERATOR_H_

#include "DocumentIterator.h"
#include "TermDocumentIterator.h"
#include "common/type_defs.h"
#include <ranking-manager/RankingManager.h>
#include <ranking-manager/RankDocumentProperty.h>

#include <ir/index_manager/utility/PriorityQueue.h>

#include <vector>
#include <map>
#include <boost/thread.hpp>

namespace sf1r
{

class WANDDocumentIterator : public DocumentIterator
{
    typedef UpperBoundInProperties::const_iterator property_name_term_index_iterator;
    typedef ID_FREQ_MAP_T::const_iterator term_index_ub_iterator;

public:
    WANDDocumentIterator(
        const property_weight_map& propertyWeightMap,
        const std::vector<unsigned int>& propertyIds,
        const std::vector<std::string>& properties
    )
        :indexPropertyList_(properties)
        ,indexPropertyIdList_(propertyIds)
    {
        init_(propertyWeightMap);
    }

    virtual ~WANDDocumentIterator();

public:

    void add(TermDocumentIterator* pDocIterator);

    void add(DocumentIterator* pDocIterator) {}

    void set_ub(bool useOriginalQuery, UpperBoundInProperties& ubmap);

    void init_threshold(float threshold);

    void setThreshold (float realThreshold);

    bool next();

    docid_t doc()
    {
        return currDoc_;
    }

    void doc_item(RankDocumentProperty& rankDocumentProperty, unsigned propIndex = 0) {}

    void df_cmtf(
        DocumentFrequencyInProperties& dfmap,
        CollectionTermFrequencyInProperties& ctfmap,
        MaxTermFrequencyInProperties& maxtfmap);

    double score(
        const std::vector<RankQueryProperty>& rankQueryProperties,
        const std::vector<boost::shared_ptr<PropertyRanker> >& propertyRankers);

    count_t tf()
    {
        return 0;
    }

    bool empty()
    {
        return docIteratorList_.empty();
    }

#if SKIP_ENABLED
    docid_t skipTo(docid_t target);

protected:
    docid_t do_skipTo(docid_t target);
#endif

protected:
    size_t getIndexOfPropertyId_(propertyid_t propertyId);

    size_t getIndexOfProperty_(const std::string& property);

    void initDocIteratorSorter();

    void init_(const property_weight_map& propertyWeightMap);

    bool do_next();

    bool findPivot();

    bool processPrePostings(docid_t target);

protected:
    std::vector<std::string> indexPropertyList_;

    std::vector<propertyid_t> indexPropertyIdList_;

    std::vector<double> propertyWeightList_;

    std::vector<TermDocumentIterator*> docIteratorList_;

    std::multimap<docid_t, TermDocumentIterator*> docIteratorSorter_;

    docid_t currDoc_;

    float currThreshold_;

    docid_t pivotDoc_;

    boost::mutex mutex_;

    ///@brief reuse in score() for performance, so score() is not thread-safe
    RankDocumentProperty rankDocumentProperty_;
};

}

#endif /* _WAND_DOCUMENT_ITERATOR_H_ */
