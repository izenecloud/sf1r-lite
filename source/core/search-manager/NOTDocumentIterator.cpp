#include "NOTDocumentIterator.h"

using namespace sf1r;
using namespace std;

NOTDocumentIterator::NOTDocumentIterator()
{
   setNot(true);
}

NOTDocumentIterator::~NOTDocumentIterator()
{
}

void NOTDocumentIterator::add(DocumentIterator* pDocIterator)
{
    docIteratorList_.push_back(pDocIterator);
}

bool NOTDocumentIterator::next()
{
    return do_next();
}

