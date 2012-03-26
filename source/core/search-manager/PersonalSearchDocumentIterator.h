/**
 * @file PersonalSearchDocumentIterator.h
 * @author Zhongxia Li
 * @date Apr 29, 2011
 * @brief Personalized Search DocumentIterator for user profile terms
 */
#ifndef PERSONAL_SEARCH_DOCUMENT_ITERATOR_H
#define PERSONAL_SEARCH_DOCUMENT_ITERATOR_H

#include "ANDDocumentIterator.h"

namespace sf1r{

/**
 * @brief PersonalSearchDocumentIterator is designed to not affecting normal search logic and scoring.
 * It used to calculate personal score related to each document while searching,
 * personal score is used to boosting normal ranking score,
 * if personal items did not occur in a document, personal scoring is ignored.
 */
class PersonalSearchDocumentIterator : public ANDDocumentIterator
{
public:
    explicit PersonalSearchDocumentIterator(bool nextResponse)
    : nextResponse_(nextResponse)
    , hasNext_(true)
    {
        currDoc_ = 0; // needed
    }

public:
    /**
     * Next action of Personalized Search
     * @details The return value is designed to not affecting AND or OR semantic of searching:
     * if PersonalSearchDocumentIterator is a sub iterator of AND Iterator, return value is always true;
     * or if it's a sub iterator of OR Iterator, return value is always false.
     * @warning It's responsible for who use this Iterator to explicitly specify which value to return
     * when creating instance of this Iterator.
     */
    bool next()
    {
        if (hasNext_) {
            hasNext_ = ANDDocumentIterator::next();
        }
        personal_current_ = hasNext_;

        //cout << " [ PersonalSearchDocumentIterator::next() ] " << nextResponse_ << endl;
        return nextResponse_;
    }

    void doc_item(RankDocumentProperty& rankDocumentProperty)
    {
        // ignore
        //cout << " [ PersonalSearchDocumentIterator::doc_item() ] " << endl;
    }

    void df_cmtf(
            DocumentFrequencyInProperties& dfmap,
            CollectionTermFrequencyInProperties& ctfmap,
            MaxTermFrequencyInProperties& maxtfmap)
    {
        // ignore
        //cout << " [ PersonalSearchDocumentIterator::df_cmtf() ] " << endl;
    }

#if SKIP_ENABLED
    /**
     * The return value is always target, so that not affecting normal search logic
     */
    docid_t skipTo(docid_t target)
    {
        if (target > currDoc_) {
            currDoc_ = ANDDocumentIterator::skipTo(target);
        }

        if (target == currDoc_)
            personal_current_ = true;
        else
            personal_current_ = false;

        //cout << " [ PersonalSearchDocumentIterator::skipTo() ] "<<target<<" - current:"<<currDoc_<< endl;
        return target;
    }
#endif

public:

    void queryBoosting(double& score, double& weight)
    {
        if (personal_current_)
        {
            score_method(score, weight);
        }
    }

    void print(int level=0)
    {
        cout << std::string(level*4, ' ') << "|--[ "
            << "PersonalSearchIter "<< personal_current_<<" "<<  currDoc_<<" ]"<< endl;

        DocumentIterator* pEntry;
        std::list<DocumentIterator*>::iterator iter = docIterList_.begin();
        for (; iter != docIterList_.end(); ++iter)
        {
            pEntry = (*iter);
            if (pEntry)
                pEntry->print(level+1);
        }
    }

private:
    /**
     * score method, matching.
     */
    void score_method(double& score, double& weight)
    {
        score += tf() * weight * 0.1;
        //cout << "Query Boosting for doc " << currDoc_ << " : "
        //    << tf() << " * " << weight * 0.1 << " = " << tf() * weight * 0.1 << endl;
    }

private:
    bool nextResponse_;

    bool hasNext_;
    bool personal_current_; // current_ may be changed outside
};


}

#endif /* PERSONAL_SEARCH_DOCUMENT_ITERATOR_H */
