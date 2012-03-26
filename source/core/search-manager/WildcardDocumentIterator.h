/**
 * @file sf1r/search-manager/WildcardDocumentIterator.h
 * @author Yingfeng Zhang
 * @date Created <2009-09-22>
 * @brief WildcardDocumentIterator DocumentIterator for wildcard query
 */
#ifndef WILDCARD_DOCUMENT_ITERATOR_H
#define WILDCARD_DOCUMENT_ITERATOR_H

#include "ORDocumentIterator.h"
#include "TermDocumentIterator.h"

namespace sf1r{

class WildcardDocumentIterator : public ORDocumentIterator
{
    class WildcardDocumentIteratorQueue : public izenelib::ir::indexmanager::PriorityQueue<TermDocumentIterator*>
    {
    public:
        WildcardDocumentIteratorQueue(size_t size)
        {
            initialize(size,true);
        }
    protected:
        bool lessThan(TermDocumentIterator* o1, TermDocumentIterator* o2)
        {
            return o1->df() < o2->df();
        }
    };

public:
    WildcardDocumentIterator(
            collectionid_t colID,
            izenelib::ir::indexmanager::IndexReader* pIndexReader,
            const std::string& property,
            unsigned int propertyId,
            bool readPositions,
            int maxTerms);

    ~WildcardDocumentIterator();

public:
    void add(termid_t termId,
                     unsigned termIndex,
                     std::map<termid_t, std::vector<izenelib::ir::indexmanager::TermDocFreqs*> >& termDocReaders);

    bool next();

    void df_cmtf(
            DocumentFrequencyInProperties& dfmap,
            CollectionTermFrequencyInProperties& ctfmap,
            MaxTermFrequencyInProperties& maxtfmap);

    unsigned int numIterators();

    void getTermIds(std::vector<termid_t>& termIds);

protected:
    void initDocIteratorQueue();

    void add(TermDocumentIterator* pDocIterator);

private:
    ///Since we adopts Document-At-A-Time query, it is not efficient to query too many terms
    ///concurrently, this private member variable is used to limit the max number of matching terms
    int maxTermThreshold_;

    ///it is necessary because we only need top frequent terms in wildcard search
    WildcardDocumentIteratorQueue* pWildcardDocIteratorQueue_;

    collectionid_t colID_;

    izenelib::ir::indexmanager::IndexReader* pIndexReader_;

    izenelib::ir::indexmanager::TermReader* pTermReader_;

    std::string property_;

    unsigned int propertyId_;

    bool readPositions_;
};

}

#endif
