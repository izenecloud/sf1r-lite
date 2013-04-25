/**
 * \file FilterDocumentIterator.h
 * \brief
 * \date Nov 17, 2011
 * \author Xin Liu
 */

#ifndef FILTER_DOCUMENT_ITERATOR_H_
#define FILTER_DOCUMENT_ITERATOR_H_

#include "DocumentIterator.h"
#include <ir/index_manager/index/TermDocFreqs.h>

#include <string>

namespace sf1r
{

class FilterDocumentIterator : public DocumentIterator
{
public:
    FilterDocumentIterator(izenelib::ir::indexmanager::TermDocFreqs* termDocFreqs)
        : termDocFreqs_(termDocFreqs)
    {
    }

    ~FilterDocumentIterator()
    {
        delete termDocFreqs_;
    }

public:
    void add(DocumentIterator* pDocIterator) {}

    bool next()
    {
        return termDocFreqs_->next();
    }

    docid_t doc()
    {
        return termDocFreqs_->doc();
    }

#if SKIP_ENABLED
    docid_t skipTo(docid_t target)
    {
        return termDocFreqs_->skipTo(target);
    }
#endif

    void doc_item(RankDocumentProperty& rankDocumentProperty, unsigned propIndex = 0) {}

    void df_cmtf(
        DocumentFrequencyInProperties& dfmap,
        CollectionTermFrequencyInProperties& ctfmap,
        MaxTermFrequencyInProperties& maxtfmap)
    {}

    count_t tf()
    {
        return termDocFreqs_->freq();
    }

    void print(int level = 0) {}


protected:
    izenelib::ir::indexmanager::TermDocFreqs* termDocFreqs_;
};

}

#endif /* FILTER_DOCUMENT_ITERATOR_H_ */
