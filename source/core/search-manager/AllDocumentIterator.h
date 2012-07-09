#ifndef CORE_SEARCH_MANAGER_ALL_DOCUMENT_ITERATOR_H_
#define CORE_SEARCH_MANAGER_ALL_DOCUMENT_ITERATOR_H_

#include "DocumentIterator.h"
#include <ir/index_manager/utility/BitMapIterator.h>

#include <string>

namespace sf1r {
using izenelib::ir::indexmanager::BitMapIterator;

/////////////////////////////////////////
// Iterate all document together with deleted doc bitmap
// The implementation is similar with ANDDocIterator,
// We don't inherit from ANDDocIterator for higher performance
// Since the number of iteration of AllDocumentIterator is
// much higher than general ANDDocIterator
class AllDocumentIterator : public DocumentIterator
{
public:
    AllDocumentIterator (docid_t maxDoc)
        : delDocIterator_(NULL)
        , currDoc_(0)
        , maxDoc_(maxDoc){}
    AllDocumentIterator (BitMapIterator* bitMapIterator, docid_t maxDoc)
        : delDocIterator_(bitMapIterator)
        , currDoc_(0)
        , maxDoc_(maxDoc)
    {
        if(delDocIterator_->next())
            currDelDoc_ = delDocIterator_->doc();
        else
            currDelDoc_ = MAX_DOC_ID;
    }

    ~AllDocumentIterator () {
        if(delDocIterator_) delete delDocIterator_;
    }

public:
    void add(DocumentIterator* pDocIterator) {}

    bool next() {
        ++currDoc_;
        if(!delDocIterator_) {
            return currDoc_ <= maxDoc_ ? true:false;
        }
        if (currDoc_ == currDelDoc_) {
            return move_with_del();
        }
        else if (currDoc_ < currDelDoc_) {
            return currDoc_ <= maxDoc_ ? true:false;
        }
        else
        {
            currDelDoc_ = delDocIterator_->skipTo(currDoc_);
            if (currDoc_ == currDelDoc_)
                return move_with_del();
            else {
                return currDoc_ <= maxDoc_ ? true:false;
            }
        }
    }

    docid_t doc()
    {
        return currDoc_;
    }

    docid_t skipTo(docid_t target) {
        if(!delDocIterator_) {
            return do_skipTo(target);
        }
        if (currDelDoc_ <= target) {
            return skip_with_del(target);
        }
        else {
            currDoc_ = currDoc_ > target ? currDoc_ : ++target;
            while((currDoc_ == currDelDoc_)&&(currDelDoc_ != MAX_DOC_ID))
            {
                currDelDoc_ = delDocIterator_->skipTo(currDoc_);
                ++ currDoc_;
            }
            return currDoc_ > maxDoc_ ? MAX_DOC_ID : currDoc_;
        }
    }

    void doc_item(RankDocumentProperty& rankDocumentProperty, unsigned propIndex = 0) {}

    void df_ctf(DocumentFrequencyInProperties& dfmap,
                     CollectionTermFrequencyInProperties& ctfmap){}

    void df_cmtf(DocumentFrequencyInProperties& dfmap,
                     CollectionTermFrequencyInProperties& ctfmap,
                     MaxTermFrequencyInProperties& maxtfmap) {}

    count_t tf() {
        return 1;
    }

protected:
    docid_t do_skipTo(docid_t target) {
        currDoc_ = currDoc_ > target ? currDoc_ : ++target;
        return currDoc_ > maxDoc_ ? MAX_DOC_ID : currDoc_;
    }

    bool move_with_del() {
        do
        {
            ++currDoc_;
            if (delDocIterator_->next())
                currDelDoc_ = delDocIterator_->doc();
            else
                currDelDoc_ = MAX_DOC_ID;
        }
        while ((currDoc_ == currDelDoc_)&&(currDoc_ <= maxDoc_));
        return currDoc_ <=maxDoc_ ? true:false;
    }

    docid_t skip_with_del(docid_t target) {
        do
        {
            currDoc_ = currDoc_ > target ? currDoc_ : ++target;
            currDelDoc_ = delDocIterator_->skipTo(currDoc_);
        }
        while ((currDoc_ == currDelDoc_)&&(currDoc_ <= maxDoc_));
        return currDoc_ > maxDoc_ ? MAX_DOC_ID : currDoc_;
    }


protected:
    BitMapIterator* delDocIterator_;

    docid_t currDoc_;

    docid_t currDelDoc_;

    docid_t maxDoc_;
};

}

#endif /* CORE_SEARCH_MANAGER_ALL_DOCUMENT_ITERATOR_H_ */
