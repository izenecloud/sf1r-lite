/**
 * \file FilterDocumentIterator.h
 * \brief
 * \date Nov 17, 2011
 * \author Xin Liu
 */

#ifndef FILTER_DOCUMENT_ITERATOR_H_
#define FILTER_DOCUMENT_ITERATOR_H_

#include "DocumentIterator.h"
#include <ir/index_manager/utility/BitMapIterator.h>

#include <string>

namespace sf1r
{

class FilterDocumentIterator : public DocumentIterator
{
public:
    FilterDocumentIterator(izenelib::ir::indexmanager::BitMapIterator* bitMapIterator)
        : bitMapIterator_(bitMapIterator)
    {
    }

    ~FilterDocumentIterator()
    {
        delete bitMapIterator_;
    }

public:
    void add(DocumentIterator* pDocIterator) {}

    bool next()
    {
        return bitMapIterator_->next();
    }

    docid_t doc()
    {
        return bitMapIterator_->doc();
    }

#if SKIP_ENABLED
    docid_t skipTo(docid_t target)
    {
        return bitMapIterator_->skipTo(target);
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
        return bitMapIterator_->freq();
    }

    void print(int level = 0) {}


protected:
    izenelib::ir::indexmanager::BitMapIterator* bitMapIterator_;
};

}

#endif /* FILTER_DOCUMENT_ITERATOR_H_ */
