/**
 * @author Wei Cao
 * @date 2009-10-10
 * @brief DocumentIterator for exact query
 */

#ifndef _EXACT_PHRASE_DOCUMENT_ITERATOR_H_
#define _EXACT_PHRASE_DOCUMENT_ITERATOR_H_

#include "PhraseDocumentIterator.h"

using namespace std;
using namespace izenelib::ir::indexmanager;

//#define VERBOSE_SERACH_MANAGER

namespace sf1r
{

///DocumentIterator for exact query.
///Template parameter IndexReaderType is added for writing unitests,
///so I can pass MockIndexReaderWriter into it.
    template<typename IndexReaderType>
    class ExactPhraseDocumentIterator_: public PhraseDocumentIterator_<IndexReaderType>
    {
    public:

        ExactPhraseDocumentIterator_(
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
            std::cout << "ExactPhraseDocumentIterator is initialized" << std::endl ;
#endif
        }

        virtual ~ExactPhraseDocumentIterator_()
        {
            if (ptrs_) delete[] ptrs_;
        }

    protected:

        virtual bool init()
        {
            if ( !PhraseDocumentIterator_<IndexReaderType>::init() )
                return false;
            PhraseDocumentIterator_<IndexReaderType>::offsets_.resize(this->termDocReaders_.size());
            ptrs_ = new size_t[PhraseDocumentIterator_<IndexReaderType>::offsets_.size()];
            return true;
        }

        /**
         * Determine whether this is an EXACT match.
         * @return true - if EXACT match.
         */
        bool isQualified()
        {
#ifdef VERBOSE_SERACH_MANAGER
            std::cout << "positions:" << std::endl;
#endif
            // Now, all TermPositions in termDocReaders_ list point to the same document.
            for (size_t i=0; i<PhraseDocumentIterator_<IndexReaderType>::offsets_.size(); i++ )
            {
                ptrs_[i] = 0;
                PhraseDocumentIterator_<IndexReaderType>::offsets_[i].clear();
                TermPositions* pPositions = this->termDocReaders_[i];
                while (true)
                {
                    loc_t o = pPositions->nextPosition();
                    if (o==BAD_POSITION)break;
#ifdef VERBOSE_SERACH_MANAGER
                    std::cout << o << ",";
#endif
                    PhraseDocumentIterator_<IndexReaderType>::offsets_[i].push_back(o);
                }
#ifdef VERBOSE_SERACH_MANAGER
                std::cout << std::endl;
#endif
                if (PhraseDocumentIterator_<IndexReaderType>::offsets_[i].empty())
                    return false;
            }
#if 1
///by Yingfeng 2010-04-15
///Currently, query term for phrase search will also be analyzed.
///For korea sentences, the blank exists, the analyzed results for each text unit (seperated by blank)
///always have same word offset, and the analyzed results for next text unit always have word offset
///just one more than previous text unit.
///While in chinese sentences, the analyzed results guarantee the incrementall word offsets
///Therefore, exact search is some kind of OrderPhrase search, except that we keep the distance for
///adjacent analyzed terms not larger than 1. This solution might still return wrong results but we do
///not have better solutions till now
            // always start from the first term
            size_t start = PhraseDocumentIterator_<IndexReaderType>::offsets_[0][ptrs_[0]];
            while (true)
            {
                // check positions from the second term to the last term
                size_t i = 1;
                for ( ; i < PhraseDocumentIterator_<IndexReaderType>::offsets_.size(); i++ )
                {
                    // forward until reach the first term
                    while ( ptrs_[i] < PhraseDocumentIterator_<IndexReaderType>::offsets_[i].size() && PhraseDocumentIterator_<IndexReaderType>::offsets_[i][ptrs_[i]] < start )
                        ptrs_[i] ++;
                    // the ith's largest position is still less than destination, so fail to match
                    if (ptrs_[i] == PhraseDocumentIterator_<IndexReaderType>::offsets_[i].size() )
                        return false;
                    if (PhraseDocumentIterator_<IndexReaderType>::offsets_[i][ptrs_[i]] > (start + 1))
                    {
                        // it couldn't be an exact match in this round, so prepare for the next round
                        ptrs_[0]++;
                        if ( ptrs_[0] == PhraseDocumentIterator_<IndexReaderType>::offsets_[0].size() )
                            return false;
                        start = PhraseDocumentIterator_<IndexReaderType>::offsets_[0][ptrs_[0]];
                        break;
                    }
                    else
                    {
                        start = PhraseDocumentIterator_<IndexReaderType>::offsets_[i][ptrs_[i]];
                    }
                }
                if (i == PhraseDocumentIterator_<IndexReaderType>::offsets_.size()) return true;
            }
#else
            // always start from the first term
            size_t start = PhraseDocumentIterator_<IndexReaderType>::offsets_[0][ptrs_[0]];
            while (true)
            {
                // check positions from the second term to the last term
                size_t i = 1;
                for ( ; i < PhraseDocumentIterator_<IndexReaderType>::offsets_.size(); i++ )
                {
                    // forward until reach the first term
                    while ( ptrs_[i] < PhraseDocumentIterator_<IndexReaderType>::offsets_[i].size() && PhraseDocumentIterator_<IndexReaderType>::offsets_[i][ptrs_[i]] < start + i )
                        ptrs_[i] ++;
                    // the ith's largest position is still less than destination, so fail to match
                    if (ptrs_[i] == PhraseDocumentIterator_<IndexReaderType>::offsets_[i].size() )
                        return false;
                    // the iths's term position is adjacent to (i-1)th's
                    if (PhraseDocumentIterator_<IndexReaderType>::offsets_[i][ptrs_[i]] == start + i)
                        continue;

                    // it couldn't be an exact match in this round, so prepare for the next round
                    ptrs_[0]++;
                    if ( ptrs_[0] == PhraseDocumentIterator_<IndexReaderType>::offsets_[0].size() )
                        return false;
                    start = PhraseDocumentIterator_<IndexReaderType>::offsets_[0][ptrs_[0]];
                    break;
                }
                if (i == PhraseDocumentIterator_<IndexReaderType>::offsets_.size()) return true;
            }
#endif
            throw std::runtime_error("ExactPhraseDocumentIterator, in an unreachable position");
        }

    protected:

        size_t* ptrs_;

    };

    typedef ExactPhraseDocumentIterator_<IndexReader> ExactPhraseDocumentIterator;

}

#undef VERBOSE_SERACH_MANAGER

#endif
