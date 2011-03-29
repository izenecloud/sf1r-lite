/**
 * @file sf1r/search-manager/OrderedDocumentIterator.h
 * @author Wei Cao
 * @date Created <2010-06-21>
 * @brief DocumentIterator which processes the out-term wildcard query
 */
#ifndef _WILDCARD_PHRASE_DOCUMENT_ITERATOR_H_
#define _WILDCARD_PHRASE_DOCUMENT_ITERATOR_H_

#include "PhraseDocumentIterator.h"

using namespace std;
using namespace izenelib::ir::indexmanager;

// #define VERBOSE_SERACH_MANAGER

namespace sf1r
{

///DocumentIterator for a phrase.
///Template parameter IndexReaderType is added for writing unitests,
///so I can pass MockIndexReaderWriter into it.
    template<typename IndexReaderType>
    class WildcardPhraseDocumentIterator_: public PhraseDocumentIterator_<IndexReaderType>
    {
    public:

        WildcardPhraseDocumentIterator_(
            std::vector<termid_t> termids,
            std::vector<unsigned> termIndexes,
            std::vector<size_t> asterisk_positions,
            std::vector<size_t> question_mark_positions,
            collectionid_t colID,
            IndexReaderType* pIndexReader,
            const string& property,
            unsigned int propertyId,
            boost::shared_ptr<DocumentManager> docMgrPtr
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
        , docMgrPtr_(docMgrPtr)
        , ptrs_(NULL)
        {
            asterisk_mask_.resize(termids.size()+1, false);
            for (size_t i=0; i<asterisk_positions.size(); i++)
                asterisk_mask_[asterisk_positions[i]] = true;

            question_mark_count_.resize(termids.size()+1, 0);
            for (size_t i=0; i<question_mark_positions.size(); i++)
                question_mark_count_[question_mark_positions[i]] ++;

#ifdef VERBOSE_SERACH_MANAGER
            std::cout << "WildcardPhraseDocumentIterator is initialized" << std::endl ;
#endif
        }

        virtual ~WildcardPhraseDocumentIterator_()
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
         * Wildcard query like "ABC*DE*F?G" is regarded as an ordered query of exact query "ABC", "DE" and "F?G",
         * question mark can be regarded as part of an exact query.
         * @return true - if ORDERED match.
         */
        bool isQualified()
        {
#ifdef VERBOSE_SERACH_MANAGER
            izenelib::util::UString raw;
            std::string str;
            int64_t value;
            docMgrPtr_->getPropertyValue( this->currDoc_, "DOCID", raw);
            raw.convertString(str, izenelib::util::UString::UTF_8);
            value = boost::lexical_cast<int64_t>(str);
            std::cout << "DOCID: " <<  value << std::endl;
            std::cout << "positions:" << std::endl;
#endif
            // Now, all TermPositions in positions_ list point to the same document.
            for (size_t i=0; i<PhraseDocumentIterator_<IndexReaderType>::offsets_.size(); i++ )
            {
                ptrs_[i] = 0;
                PhraseDocumentIterator_<IndexReaderType>::offsets_[i].clear();
                TermPositions* pPositions =
                    (TermPositions*)this->termDocReaders_[i];
                while (true)
                {
                    loc_t o = pPositions->nextPosition();
                    //this->termDocReaders_[i]->nextPosition();
                    if (o==BAD_POSITION)break;
#ifdef VERBOSE_SERACH_MANAGER
                    std::cout << o << ",";
#endif
                    PhraseDocumentIterator_<IndexReaderType>::offsets_[i].push_back(o);
                }
#ifdef VERBOSE_SERACH_MANAGER
                std::cout << std::endl;
#endif
            }

            // start from the first term of the left most exact query
            size_t pos = 0;
            while (true)
            {
                if ( ptrs_[pos] == PhraseDocumentIterator_<IndexReaderType>::offsets_[pos].size() ) return false;
                size_t start = PhraseDocumentIterator_<IndexReaderType>::offsets_[pos][ptrs_[pos]];

                // check positions from the second term to the last term in this exact query
                size_t i = pos+1;
                size_t offset = 1;

                for ( ; i < PhraseDocumentIterator_<IndexReaderType>::offsets_.size(); i++, offset++ )
                {
                    // additional character for question mark
                    offset += question_mark_count_[i];

                    // forward until reach the first term
                    while ( ptrs_[i] < PhraseDocumentIterator_<IndexReaderType>::offsets_[i].size() && PhraseDocumentIterator_<IndexReaderType>::offsets_[i][ptrs_[i]] < start + offset )
                        ptrs_[i] ++;
                    // the ith's largest position is still less than destination, so fail to match
                    if (ptrs_[i] == PhraseDocumentIterator_<IndexReaderType>::offsets_[i].size() ) {
                        return false;
                    }

                    // find the end of this exact search and init next exact search
                    if( asterisk_mask_[i] ) {
                        pos = i;
                        break;
                    }
                    // the iths's term position is adjacent to (i-1)th's
                    if (PhraseDocumentIterator_<IndexReaderType>::offsets_[i][ptrs_[i]] == start + offset)
                        continue;

                    // it couldn't be an exact match in this round, so prepare for the next round
                    ptrs_[pos]++;
                    break;
                }
                if (i == PhraseDocumentIterator_<IndexReaderType>::offsets_.size()) return true;
            }
        }

    protected:

        std::vector<bool> asterisk_mask_;

        std::vector<int> question_mark_count_;

        boost::shared_ptr<DocumentManager> docMgrPtr_;

        size_t* ptrs_;

    };

    typedef WildcardPhraseDocumentIterator_<IndexReader> WildcardPhraseDocumentIterator;

}

#undef VERBOSE_SERACH_MANAGER

#endif
