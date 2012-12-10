/**
 * @file sf1r/search-manager/NOTDocumentIterator.h
 * @author Yingfeng Zhang
 * @date Created <2009-09-23>
 * @brief DocumentIterator which implements the NOT semantics
 */
#ifndef NOT_DOCUMENT_ITERATOR_H
#define NOT_DOCUMENT_ITERATOR_H

#include "ORDocumentIterator.h"

namespace sf1r
{

class NOTDocumentIterator : public ORDocumentIterator
{
public:
    NOTDocumentIterator();

    ~NOTDocumentIterator();
public:
    void add(DocumentIterator* pDocIterator);

    bool next();
};

}

#endif
