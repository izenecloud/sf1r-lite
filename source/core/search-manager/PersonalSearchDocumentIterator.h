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
    : nextResponse_(nextResponse) {}

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
        // do next
        ANDDocumentIterator::next();

        return nextResponse_;
    }

    double score()
    {
        return 0;
    }

private:
    bool nextResponse_;
};


}

#endif /* PERSONAL_SEARCH_DOCUMENT_ITERATOR_H */
