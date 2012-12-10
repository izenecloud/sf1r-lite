/**
 * @file sf1r/search-manager/ANDDocumentIterator.h
 * @author Yingfeng Zhang
 * @date Created <2009-09-22>
 * @date Updated <2010-03-24 14:41:29>
 * @brief DocumentIterator which implements the AND semantics
 * NOT semantics is also included
 */
#ifndef AND_DOCUMENT_ITERATOR_H
#define AND_DOCUMENT_ITERATOR_H

#include "DocumentIterator.h"
#include "NOTDocumentIterator.h"

#include <list>

namespace sf1r
{

class ANDDocumentIterator:public DocumentIterator
{
public:
    ANDDocumentIterator();

    ~ANDDocumentIterator();

public:
    void add(DocumentIterator* pDocIterator);

    void add(VirtualPropertyTermDocumentIterator* pDocIterator);

    bool next()
    {
        if (!hasNot_)
            return do_next();
        else
        {
            if (docIterList_.empty())
                return false;
            if (! initNOTIterator_)
            {
                initNOTIterator_ = true;
                if (pNOTDocIterator_->next())
                    currDocOfNOTIter_ = pNOTDocIterator_->doc();
                else
                    currDocOfNOTIter_ = MAX_DOC_ID;
            }

            bool ret = do_next();
            if (currDoc_ == currDocOfNOTIter_)
            {
                return move_together_with_not();
            }
            else if (currDoc_ < currDocOfNOTIter_)
                return ret;
            else
            {
                currDocOfNOTIter_ = pNOTDocIterator_->skipTo(currDoc_);
                if (currDoc_ == currDocOfNOTIter_)
                    return move_together_with_not();
                else
                    return ret;
            }
        }
    }

    docid_t doc()
    {
        return currDoc_;
    }

    inline void doc_item(RankDocumentProperty& rankDocumentProperty, unsigned propIndex = 0);

    void df_cmtf(
        DocumentFrequencyInProperties& dfmap,
        CollectionTermFrequencyInProperties& ctfmap,
        MaxTermFrequencyInProperties& maxtfmap);

    count_t tf();

    bool empty()
    {
        return docIterList_.empty();
    }

    void queryBoosting(double& score, double& weight);

    void print(int level = 0);

#if SKIP_ENABLED
    inline docid_t skipTo(docid_t target);
private:
    inline docid_t do_skipTo(docid_t target);
#endif
private:
    inline bool do_next();

    inline bool move_together_with_not();

protected:
    docid_t currDoc_;

    bool hasNot_;

    docid_t currDocOfNOTIter_;

    bool initNOTIterator_;

    NOTDocumentIterator* pNOTDocIterator_;

    std::list<DocumentIterator*> docIterList_;

    ///Use a member to record size of docIterList, becaus std::list::size() has O(n) overheads
    size_t nIteratorNum_;
};


inline bool ANDDocumentIterator::move_together_with_not()
{
    bool ret;
    do
    {
        ret = do_next();
//        currDocOfNOTIter_ = pNOTDocIterator_->skipTo(currDoc_);
        if (pNOTDocIterator_->next())
            currDocOfNOTIter_ = pNOTDocIterator_->doc();
        else
            currDocOfNOTIter_ = MAX_DOC_ID;
    }
    while (ret&&(currDoc_ == currDocOfNOTIter_));
    return ret;
}

inline bool ANDDocumentIterator::do_next()
{
    if (!nIteratorNum_ ||(docIterList_.front()->next()==false))
        return false;
    if (nIteratorNum_ == 1)
    {
        currDoc_ = docIterList_.front()->doc();
        return true;
    }
    docid_t target,nearTarget;

    target = docIterList_.front()->doc();
    docIterList_.push_back(docIterList_.front());
    docIterList_.pop_front();

    size_t nMatch = 1;

    while ( nMatch < nIteratorNum_ )
    {
        nearTarget = docIterList_.front()->skipTo(target);
        if (nearTarget == target)
            nMatch++;
        else
        {
            nMatch = 1;
            if (nearTarget == MAX_DOC_ID)
                break;
            if (nearTarget > target)
            {
                target = nearTarget;
            }
            else break;
        }
        //firstToLast;
        docIterList_.push_back(docIterList_.front());
        docIterList_.pop_front();
    }
    if (nMatch == nIteratorNum_)
    {
        currDoc_ = target;
        return true;
    }
    return false;

}

#if SKIP_ENABLED
inline docid_t ANDDocumentIterator::skipTo(docid_t target)
{
    if (!hasNot_)
        return do_skipTo(target);
    else
    {
        docid_t nFoundId, currentDoc = target;
        do
        {
            nFoundId = do_skipTo(currentDoc);
            currDocOfNOTIter_ = pNOTDocIterator_->skipTo(currentDoc);
            ///skipto and next have different semantics:
            ///for next, if it does not have value, it will return false.
            ///while for skipto, if the target is the same as current last doc,
            ///it will still return target
            if((nFoundId != MAX_DOC_ID) && ((nFoundId == currentDoc) &&(currDocOfNOTIter_ == currentDoc)))
                return MAX_DOC_ID;
            currentDoc = nFoundId;
        }
        while ((nFoundId != MAX_DOC_ID)&&(nFoundId == currDocOfNOTIter_));
        return nFoundId;
    }
}

inline docid_t ANDDocumentIterator::do_skipTo(docid_t target)
{
    docid_t nFoundId = MAX_DOC_ID;
    size_t nMatch = 0;
    size_t nIteratorNum = docIterList_.size();
    while ( nMatch < nIteratorNum)
    {
        nFoundId = docIterList_.front()->skipTo(target);
        if(MAX_DOC_ID == nFoundId)
            break;
        else if(nFoundId == target)
            nMatch++;
        else if(nFoundId > target)
        {
            target = nFoundId;
            nMatch = 1;
        }
        else break;

        docIterList_.push_back(docIterList_.front());
        docIterList_.pop_front();
    }
    if(nMatch == nIteratorNum)
    {
        currDoc_ = target;
        return nFoundId;
    }
    return MAX_DOC_ID;
}
#endif

inline void ANDDocumentIterator::doc_item(
    RankDocumentProperty& rankDocumentProperty,
    unsigned propIndex)
{
    DocumentIterator* pEntry;
    std::list<DocumentIterator*>::iterator iter = docIterList_.begin();
    for (; iter != docIterList_.end(); ++iter)
    {
        pEntry = (*iter);
        pEntry->doc_item(rankDocumentProperty, propIndex);
    }
}

}

#endif
