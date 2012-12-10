#ifndef _ORDERED_PHRASE_DOCUMENT_ITERATOR_H_
#define _ORDERED_PHRASE_DOCUMENT_ITERATOR_H_

#include "PhraseDocumentIterator.h"

using namespace std;
using namespace izenelib::ir::indexmanager;

//#define VERBOSE_SERACH_MANAGER

namespace sf1r
{

///DocumentIterator for a phrase.
///Template parameter IndexReaderType is added for writing unitests,
///so I can pass MockIndexReaderWriter into it.
template<typename IndexReaderType>
class OrderedPhraseDocumentIterator_: public PhraseDocumentIterator_<IndexReaderType>
{
public:

    OrderedPhraseDocumentIterator_(
        std::vector<termid_t> termids,
        std::vector<unsigned> termIndexes,
        collectionid_t colID,
        IndexReaderType* pIndexReader,
        const string& property,
        unsigned int propertyId
    )
        : PhraseDocumentIterator_<IndexReaderType>(
            termids,
            termIndexes,
            colID,
            pIndexReader,
            property,
            propertyId,
            true
        )
        , ptrs_(NULL)
    {
#ifdef VERBOSE_SERACH_MANAGER
        std::cout << "OrderedPhraseDocumentIterator is initialized" << std::endl ;
#endif
    }

    virtual ~OrderedPhraseDocumentIterator_()
    {
        if(ptrs_) delete[] ptrs_;
    }

protected:

    virtual bool init()
    {
        if( !PhraseDocumentIterator_<IndexReaderType>::init() )
            return false;
        PhraseDocumentIterator_<IndexReaderType>::offsets_.resize(this->termDocReaders_.size());
        ptrs_ = new size_t[PhraseDocumentIterator_<IndexReaderType>::offsets_.size()];
        return true;
    }

    /**
     * Determine whether this is an ORDERED match.
     * @return true - if ORDERED match.
     */
    bool isQualified()
    {
#ifdef VERBOSE_SERACH_MANAGER
        std::cout << "positions:" << std::endl;
#endif
        // Now, all TermPositions in positions_ list point to the same document.
        for(size_t i=0; i<PhraseDocumentIterator_<IndexReaderType>::offsets_.size(); i++ )
        {
            ptrs_[i] = 0;
            PhraseDocumentIterator_<IndexReaderType>::offsets_[i].clear();
            TermPositions* pPositions =
                (TermPositions*)this->termDocReaders_[i];
            while(true)
            {
                loc_t o = pPositions->nextPosition();
                //this->termDocReaders_[i]->nextPosition();
                if(o==BAD_POSITION)break;
#ifdef VERBOSE_SERACH_MANAGER
                std::cout << o << ",";
#endif
                PhraseDocumentIterator_<IndexReaderType>::offsets_[i].push_back(o);
            }
#ifdef VERBOSE_SERACH_MANAGER
            std::cout << std::endl;
#endif
        }

        size_t step = PhraseDocumentIterator_<IndexReaderType>::offsets_[0][0];
        for( size_t i = 1 ; i < PhraseDocumentIterator_<IndexReaderType>::offsets_.size(); i++ )
        {
            while( ptrs_[i] < PhraseDocumentIterator_<IndexReaderType>::offsets_[i].size() && PhraseDocumentIterator_<IndexReaderType>::offsets_[i][ptrs_[i]] < step)
                ptrs_[i] ++;
            if(ptrs_[i] == PhraseDocumentIterator_<IndexReaderType>::offsets_[i].size() )
                return false;
            step = PhraseDocumentIterator_<IndexReaderType>::offsets_[i][ptrs_[i]];
        }
        return true;
    }

protected:

    size_t* ptrs_;

};

typedef OrderedPhraseDocumentIterator_<IndexReader> OrderedPhraseDocumentIterator;

}

#undef VERBOSE_SERACH_MANAGER

#endif
