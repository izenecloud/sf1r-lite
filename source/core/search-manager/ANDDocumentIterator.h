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

namespace sf1r{

class ANDDocumentIterator:public DocumentIterator
{
public:
    ANDDocumentIterator();

    ~ANDDocumentIterator();

public:
    void add(DocumentIterator* pDocIterator);

    bool next();

    docid_t doc() { return currDoc_; }

    void doc_item(RankDocumentProperty& rankDocumentProperty);

    void df_ctf(DocumentFrequencyInProperties& dfmap, CollectionTermFrequencyInProperties& ctfmap);

    count_t tf();

    bool empty() { return docIterList_.empty(); }

    void queryBoosting(double& score, double& weight);

    void print(int level = 0);

#if SKIP_ENABLED
    docid_t skipTo(docid_t target);
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
    docid_t target,nearTarget;
    size_t nIteratorNum = docIterList_.size();

    if (docIterList_.empty()||(docIterList_.front()->next()==false))
        return false;

    target = docIterList_.front()->doc();
    docIterList_.push_back(docIterList_.front());
    docIterList_.pop_front();

    size_t nMatch = 1;

    while ( nMatch < nIteratorNum )
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
    if (nMatch == nIteratorNum)
    {
        currDoc_ = target;
        return true;
    }
    return false;

}

}

#endif
