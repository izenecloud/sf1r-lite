/**
 * @file SearchTermDocumentIterator.h
 * @author Zhongxia Li
 * @date Jun 16, 2011
 * @brief Search Terms used for searching (retrieving) documents, but not for ranking.
 * SearchTermDocumentIterator performs next()/skipTo() normally, but do nothing for collecting rank inforamtion.
 */
#ifndef SEARCH_TERM_DOCUMENT_ITERATOR_H_
#define SEARCH_TERM_DOCUMENT_ITERATOR_H_

#include "TermDocumentIterator.h"

namespace sf1r{

class SearchTermDocumentIterator : public TermDocumentIterator
{
public:
    SearchTermDocumentIterator(
        termid_t termid,
        collectionid_t colID,
        IndexReader* pIndexReader,
        const std::string& property,
        unsigned int propertyId,
        unsigned int termIndex,
        bool readPositions)
    : TermDocumentIterator(termid, colID, pIndexReader, property, propertyId, termIndex, readPositions)
    {
    }

public:
    /*virtual*/
    void df_cmtf(
            DocumentFrequencyInProperties& dfmap,
            CollectionTermFrequencyInProperties& ctfmap,
            MaxTermFrequencyInProperties& maxtfmap)
    {
        // do nothing
    }

    /*virtual*/
    void doc_item(RankDocumentProperty& rankDocumentProperty, unsigned propIndex = 0)
    {
        // do nothing
    }

public:
    void print(int level=0)
    {
        cout << std::string(level*4, ' ') << "|--[ "
                << "SearchTermIter " << current_
                << " - termid: " << termId_ << " " << property_<<" ]"<< endl;
    }
};

}


#endif /* SEARCH_TERM_DOCUMENT_ITERATOR_H_ */
