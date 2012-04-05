/**
 * @file RankTermDocumentIterator.h
 * @author Zhongxia Li
 * @date Jun 16, 2011
 * @brief Rank Terms used for ranking, but not for searching(retrieving) documents.
 * RankTermDocumentIterator perform next()/skipTo() action, but not involved in judging the end condition of next() when searching.
 * It collects rank information normally.
 */
#ifndef RANK_TERM_DOCUMENT_ITERATOR_H
#define RANK_TERM_DOCUMENT_ITERATOR_H

#include "TermDocumentIterator.h"

namespace sf1r{

class RankTermDocumentIterator : public TermDocumentIterator
{
public:

    explicit RankTermDocumentIterator(
            termid_t termid,
            collectionid_t colID,
            izenelib::ir::indexmanager::IndexReader* pIndexReader,
            const std::string& property,
            unsigned int propertyId,
            unsigned int termIndex,
            bool readPositions,
            bool nextResponse)
    : TermDocumentIterator(termid, colID, pIndexReader, property, propertyId, termIndex, readPositions)
    , nextResponse_(nextResponse)
    , hasNext_(true)
    , isCurrent_(false)
    {
        currDoc_ = 0;
    }

public:
    /**
     * @details
     * - if parent Iterator is AND semantic, nextResponse is always true;
     * - if parent Iterator is OR semantic, nextResponse is always false.
     * @warning It's responsible for who use this Iterator to explicitly specify which value to return
     * when creating instance of this Iterator.
     */
    /*virtual*/
    bool next()
    {
        if (hasNext_)
        {
            hasNext_ = TermDocumentIterator::next();
            isCurrent_ = hasNext_;
        }

        //cout << " [ RankTermsDocumentIterator::next() ] " << nextResponse_ << endl;
        return nextResponse_;
    }

#if SKIP_ENABLED
    /**
     * The return value is always target, so that not affecting normal search logic
     */
    /*virtual*/
    docid_t skipTo(docid_t target)
    {
        if (target > currDoc_) {
            currDoc_ = TermDocumentIterator::skipTo(target);
        }

        if (target == currDoc_)
            isCurrent_ = true;
        else
            isCurrent_ = false;

        //cout << " [ RankTermsDocumentIterator::skipTo() ] "<<target<<" - current:"<<currDoc_<< endl;
        return target;
    }
#endif

public:
    /*virtual*/
    void df_cmtf(
            DocumentFrequencyInProperties& dfmap,
            CollectionTermFrequencyInProperties& ctfmap,
            MaxTermFrequencyInProperties& maxtfmap)
    {
        TermDocumentIterator::df_cmtf(dfmap, ctfmap, maxtfmap);
    }

    /*virtual*/
    void doc_item(RankDocumentProperty& rankDocumentProperty, unsigned propIndex)
    {
        if ( isCurrent_ ) {
            TermDocumentIterator::doc_item(rankDocumentProperty);
        }
    }

public:
    void print(int level=0)
    {
        cout << std::string(level*4, ' ') << "|--[ "
                << "RankTermIter " << isCurrent_
                << " - termid: " << termId_ << " " << property_<<" ]"<< endl;
    }

private:
    bool nextResponse_;

    bool hasNext_;

    bool isCurrent_;
};

}

#endif /* RANK_TERM_DOCUMENT_ITERATOR_H */
