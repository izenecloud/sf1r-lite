#ifndef CORE_SEARCH_MANAGER_VIRTUAL_PROPERTY_TERM_DOCUMENT_ITERATOR_H
#define CORE_SEARCH_MANAGER_VIRTUAL_PROPERTY_TERM_DOCUMENT_ITERATOR_H

#include "ORDocumentIterator.h"

#include <set>

namespace sf1r
{

class TermDocumentIterator;
class VirtualPropertyTermDocumentIterator : public ORDocumentIterator
{
public:
    VirtualPropertyTermDocumentIterator(const std::string& property);

    void add(TermDocumentIterator* pDocIterator);

    void doc_item(RankDocumentProperty& rankDocumentProperty, unsigned propIndex = 0);

    void df_cmtf(DocumentFrequencyInProperties& dfmap,
                 CollectionTermFrequencyInProperties& ctfmap,
                 MaxTermFrequencyInProperties& maxtfmap);

protected:
    std::set<propertyid_t> propIds_;
    std::string virtualProperty_;
};

}

#endif
