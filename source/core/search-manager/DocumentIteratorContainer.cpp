#include "DocumentIteratorContainer.h"
#include "DocumentIterator.h"
#include "CombinedDocumentIterator.h"

using namespace sf1r;

DocumentIteratorContainer::~DocumentIteratorContainer()
{
    for (std::vector<DocumentIterator*>::iterator it = docIters_.begin();
         it != docIters_.end(); ++it)
    {
        delete *it;
    }
}

void DocumentIteratorContainer::add(DocumentIterator* docIter)
{
    if (NULL == docIter)
    {
        hasNullDocIterator_ = true;
        return;
    }
    docIters_.push_back(docIter);
}

DocumentIterator* DocumentIteratorContainer::combine()
{
    if (docIters_.empty() || hasNullDocIterator_)
        return NULL;

    DocumentIterator* result = NULL;

    if (docIters_.size() == 1)
    {
        result = docIters_[0];
    }
    else
    {
        result = new CombinedDocumentIterator;
        for (std::vector<DocumentIterator*>::iterator it = docIters_.begin();
             it != docIters_.end(); ++it)
        {
            result->add(*it);
        }
    }

    docIters_.clear();
    return result;
}
