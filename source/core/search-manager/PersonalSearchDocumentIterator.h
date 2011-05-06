/**
 * @file PersonalSearchDocumentIterator.h
 * @author Zhongxia Li
 * @date Apr 29, 2011
 * @brief Personalized Search
 */
#ifndef PERSONAL_SEARCH_DOCUMENT_ITERATOR_H
#define PERSONAL_SEARCH_DOCUMENT_ITERATOR_H

#include "ANDDocumentIterator.h"

namespace sf1r{

class PersonalSearchDocumentIterator : public ANDDocumentIterator
{
public:
    explicit PersonalSearchDocumentIterator(bool nextResponse)
    : nextResponse_(nextResponse)
    , hasNext_(true)
    {
    }

public:
    /**
     * Personalized Search DocumentIterator for user profile terms
     * @details The return value is designed to not affecting AND or OR semantic of searching:
     * if PersonalSearchDocumentIterator is a sub iterator of AND Iterator, return value is always true;
     * or if it's a sub iterator of OR Iterator, return value is always false.
     * @warning It's responsible for who use this Iterator to explicitly specify which value to return
     * when creating instance of this Iterator.
     */
    bool next()
    {
        if (!hasNext_) {
            hasNext_ = ANDDocumentIterator::next();
        }

        cout << " [ PersonalSearchDocumentIterator::next() ] " << nextResponse_ << endl;
        return nextResponse_;
    }

    void doc_item(RankDocumentProperty& rankDocumentProperty)
    {
        // ignore
        cout << " [ PersonalSearchDocumentIterator::doc_item() ] " << endl;
    }

    void df_ctf(DocumentFrequencyInProperties& dfmap, CollectionTermFrequencyInProperties& ctfmap)
    {
        // ignore
        cout << " [ PersonalSearchDocumentIterator::df_ctf() ] " << endl;
    }

    count_t tf()
    {
        // ignore
        return 0;
    }

    void print(int level=0)
    {
        cout << std::string(level*4, ' ') << "|--[ "<< "PersonalIter " << "" << endl;

        ANDDocumentIterator::print(level);
    }

public:

    void queryBoosting(double& score, double& weight)
    {
        score_method(score, weight);
    }

private:
    /**
     * score of user profile terms to current document property.
     * @return score
     */
    void score_method(double& score, double& weight)
    {
        score += tf() * weight;
        cout << "queryBoosting: " << tf() << "  " << score << endl;
    }

private:
    bool nextResponse_;
    bool hasNext_;
};


}

#endif /* PERSONAL_SEARCH_DOCUMENT_ITERATOR_H */
