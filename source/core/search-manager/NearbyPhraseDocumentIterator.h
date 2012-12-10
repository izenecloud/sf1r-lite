#ifndef _NEARBY_PHRASE_DOCUMENT_ITERATOR_H_
#define _NEARBY_PHRASE_DOCUMENT_ITERATOR_H_

#include "PhraseDocumentIterator.h"

using namespace std;
using namespace izenelib::ir::indexmanager;

namespace sf1r
{

///DocumentIterator for a phrase.
///Template parameter IndexReaderType is added for writing unitests,
///so I can pass MockIndexReaderWriter into it.
template<typename IndexReaderType>
class NearbyPhraseDocumentIterator_: public PhraseDocumentIterator_<IndexReaderType>
{
public:

    NearbyPhraseDocumentIterator_(
        std::vector<termid_t> termids,
        std::vector<unsigned> termIndexes,
        collectionid_t colID,
        IndexReaderType* pIndexReader,
        const string& property,
        unsigned int propertyId,
        const count_t range)
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
        , range_(range)
    {
#ifdef VERBOSE_SERACH_MANAGER
        std::cout << "NearbyDocumentIterator is initialized, range: " << range_ << std::endl ;
#endif
    }

    virtual ~NearbyPhraseDocumentIterator_()
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
     * To see whether term n appers between (off, off+range_],
     * also check all following terms after term n meet the requirement.
     */
    bool findMatch(size_t n, loc_t off)
    {
        // std::cout << "<" << n << "," << off << ">" << std::endl;
        if(n == PhraseDocumentIterator_<IndexReaderType>::offsets_.size() ) return true;

        // forward PhraseDocumentIterator_<IndexReaderType>::offsets_[n][ptrs_[n]] to off+1
        while( ptrs_[n] < PhraseDocumentIterator_<IndexReaderType>::offsets_[n].size() && PhraseDocumentIterator_<IndexReaderType>::offsets_[n][ptrs_[n]] < off )
            ptrs_[n] ++;

        // for term n's each occurence inside (off, off+range_]
        while(ptrs_[n] < PhraseDocumentIterator_<IndexReaderType>::offsets_[n].size() && PhraseDocumentIterator_<IndexReaderType>::offsets_[n][ptrs_[n]] <= off + range_ )
        {
            if( findMatch( n+1, PhraseDocumentIterator_<IndexReaderType>::offsets_[n][ptrs_[n]] ) ) return true;
            ptrs_[n] ++;
        }

        return false;
    }

    /**
     * Determine whether this is an NEARBY match.
     * @return true - if NEARBY match.
     */
    bool isQualified()
    {
        // Now, all TermDocFreqs in list point to the same document.
        for(size_t i=0; i<PhraseDocumentIterator_<IndexReaderType>::offsets_.size(); i++ )
        {
            ptrs_[i] = 0;
            PhraseDocumentIterator_<IndexReaderType>::offsets_[i].clear();
            TermPositions* pPositions = this->termDocReaders_[i];
            while(true)
            {
                loc_t o = pPositions->nextPosition();
                if(o==BAD_POSITION)break;
                PhraseDocumentIterator_<IndexReaderType>::offsets_[i].push_back(o);
            }
        }
        // std::cout << "start find match" << std::endl;

        // for term0's each occurence, check whether there is a NEARBY match
        for(size_t i=0; i< PhraseDocumentIterator_<IndexReaderType>::offsets_[0].size(); i++ )
            if( findMatch(1, PhraseDocumentIterator_<IndexReaderType>::offsets_[0][i]) )
                return true;
        return false;
    }

protected:

    size_t* ptrs_;

    count_t range_;

};

typedef NearbyPhraseDocumentIterator_<IndexReader> NearbyPhraseDocumentIterator;

}

#endif
