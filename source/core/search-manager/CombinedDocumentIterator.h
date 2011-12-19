#ifndef SF1R_SEARCHMANAGER_COMBINED_DOCITERATOR_H
#define SF1R_SEARCHMANAGER_COMBINED_DOCITERATOR_H

#include "ANDDocumentIterator.h"

namespace sf1r{

// class CombinedDocumentIterator
// The eventual DocumentIterator used by SearchManager
// It's the same as ANDDocumentIterator
// FilterDocumentIterator and MultiPropertyScorer are added to its members
// The only usage for this class is to make profiling convenient
class CombinedDocumentIterator : public ANDDocumentIterator
{
public:
    CombinedDocumentIterator(){}

    ~CombinedDocumentIterator (){}

    bool next()
    {
        CREATE_SCOPED_PROFILER ( dociterating, "SearchManager", "doSearch_: doc iterating");
        return ANDDocumentIterator::next();
    }

};

}

#endif

