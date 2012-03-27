/**
 * @file sf1r/search-manager/PhraseDocumentIterator.h
 * @author Wei Cao
 * @date 2009-10-10
 * @brief PhraseDocumentIterator DocumentIterator for a phrase
 */
#ifndef PHRASE_DOCUMENT_ITERATOR_H
#define PHRASE_DOCUMENT_ITERATOR_H

#include "DocumentIterator.h"

#include <ir/index_manager/index/AbsTermReader.h>
#include <ir/index_manager/index/IndexReader.h>
#include <ir/index_manager/index/TermPositions.h>
#include <ir/index_manager/index/TermDocFreqs.h>
#include <ir/index_manager/utility/PriorityQueue.h>

#include <boost/assert.hpp>

#include <vector>
#include <string>

using namespace std;
using namespace izenelib::ir::indexmanager;

//#define VERBOSE_SERACH_MANAGER

namespace sf1r{

///DocumentIterator for a phrase.
///Template parameter IndexReaderType is added for writing unitests,
///so I can pass MockIndexReaderWriter into it.
template<typename IndexReaderType>
class PhraseDocumentIterator_: public DocumentIterator
{

public:

    /// A phrase is composited by multiple terms
    PhraseDocumentIterator_(std::vector<termid_t> termids,
                            std::vector<unsigned> termIndexes,
                            collectionid_t colID,
                            IndexReaderType* pIndexReader,
                            const string& property,
                            unsigned int propertyId,
                            bool readPositions)
        : termids_(termids),
          termIndexes_(termIndexes),
          colID_(colID),
          pIndexReader_(pIndexReader),
          property_(property),
          propertyId_(propertyId),
          origProperty_(property),
          inited_(false),
          hasNext_(false),
          readPositions_(readPositions)
    {
    }

    virtual ~PhraseDocumentIterator_()
    {
        for( size_t i = 0; i<termDocReaders_.size(); i++ )
            delete termDocReaders_[i];
    }

    void setOrigProperty(const string& property)
    {
        origProperty_ = property;
    }

    void add(DocumentIterator* pDocIterator){}

    bool next()
    {
#ifdef VERBOSE_SERACH_MANAGER
        std::cout << "next" << std::endl;
#endif
        if(!inited_) {
            hasNext_ = init();
            inited_ = true;
        }

        while (hasNext_) {
            hasNext_ = do_next();
            if(hasNext_) {
                if( isQualified() ) {
#ifdef VERBOSE_SERACH_MANAGER
                    std::cout << "match" << std::endl;
#endif
                    break;
                }
#ifdef VERBOSE_SERACH_MANAGER
                else {
                    std::cout << "miss" << std::endl;
                }
#endif
            }
        }
        return hasNext_;
    }


    docid_t doc() { return currDoc_; }

    void doc_item(RankDocumentProperty& rankDocumentProperty)
    {
        for( size_t i = 0; i < termDocReaders_.size(); i++ )
        {
            BOOST_ASSERT(termIndexes_[i] < rankDocumentProperty.size());
            if (rankDocumentProperty.termFreqAt(termIndexes_[i]) == 0)
            {
                rankDocumentProperty.setDocLength(
                    pIndexReader_->docLength(currDoc_, propertyId_)
                );

                rankDocumentProperty.activate(termIndexes_[i]);
                size_t num_pos = offsets_[i].size();

                for( size_t j = 0; j < num_pos; ++j)
                {
                    rankDocumentProperty.pushPosition(offsets_[i][j]);
                }

            }
        }
    }

    void df_cmtf(
            DocumentFrequencyInProperties& dfmap,
            CollectionTermFrequencyInProperties& ctfmap,
            MaxTermFrequencyInProperties& maxtfmap)
    {
        if(!inited_) {
            hasNext_ = init();
            inited_ = true;
        }

        for( size_t i = 0; i < termDocReaders_.size(); i++ ) {
            TermDocFreqs* pTermDocReader = termDocReaders_[i];
            ID_FREQ_MAP_T& df = dfmap[origProperty_];
            df[termids_[i]] = (float)pTermDocReader->docFreq();

            ID_FREQ_MAP_T& ctf = ctfmap[origProperty_];
            ctf[termids_[i]] = (float)pTermDocReader->getCTF();
        }
    }

    count_t tf()
    {
        // here return the minimum tf,
        //
        if(!inited_) {
            hasNext_ = init();
            inited_ = true;
        }

        TermDocFreqs* pTermDocReader;
        count_t mintf = MAX_COUNT;
        count_t tf;
        for( size_t i = 0; i < termDocReaders_.size(); i++ ) {
            pTermDocReader = termDocReaders_[i];
            tf = pTermDocReader->freq();
            if (tf < mintf) {
                mintf = tf;
            }
        }

        return mintf;
    }

protected:

    virtual bool init()
    {
        if(termids_.size() == 0) return false;

        TermReader* pTermReader = pIndexReader_->getTermReader(colID_);
        if(!pTermReader) return false;

        bool succ = true;

//        if(pIndexReader_->isDirty()) {
//            delete pTermReader; return false;
//        }

        termDocReaders_.reserve(termids_.size());
        for (size_t i = 0; i< termids_.size(); i++ ) {
            Term term(property_.c_str(),termids_[i]);
            if( ! pTermReader->seek(&term) ) {
                succ = false; break;
            }

//            if(pIndexReader_->isDirty()) {
//                delete pTermReader;return false;
//            }
            termDocReaders_.push_back(pTermReader->termPositions());
        }
        delete pTermReader;
        return succ;
    }

    /// Overriden it if strong restriction is required, for example, exact matches.
    virtual bool isQualified() { return true; }

    bool do_next()
    {
        if (termDocReaders_[0]->next()==false)
        {
            currDoc_ = MAX_DOC_ID;
            return false;
        }
        docid_t target = termDocReaders_[0]->doc();
        size_t start= 1;
        size_t nMatch = 1;
        docid_t nearTarget = MAX_DOC_ID;

        while(nMatch < termDocReaders_.size())
        {
            size_t i = start % termDocReaders_.size();
            nearTarget = termDocReaders_[i]->skipTo(target);
            if((MAX_DOC_ID == nearTarget)||(nearTarget < target))
            {
                currDoc_ = MAX_DOC_ID;
                return false;
            }
#ifdef VERBOSE_SERACH_MANAGER
            std::cout << termDocReaders_[i]->doc() << ",";
#endif
            if(nearTarget == target) {
                nMatch ++;
            } else {
                target = nearTarget;
                nMatch = 1;
            }
            ++start;
#ifdef VERBOSE_SERACH_MANAGER
            std::cout << std::endl;
#endif
        }
        if(nMatch == termDocReaders_.size()) {
#ifdef VERBOSE_SERACH_MANAGER
            std::cout << currDoc_ << std::endl;
#endif
            currDoc_ = target;
            return true;
        }
        return false;
    }

    std::vector<termid_t> termids_;

    std::vector<unsigned> termIndexes_;

    collectionid_t colID_;

    IndexReaderType* pIndexReader_;

    std::string property_;

    unsigned int propertyId_;

    ///2010-10-12  Yingfeng
    ///Very ugly here!!
    ///in some cases, when performing phrase queries, the practical query is not performed over
    ///property_, eg:  an exact query over "Content" might be applied over "Content_unigram"
    ///automatically instead of "Content", however, this automatic conversion is not known by ranker:
    ///it still think the query is performed over "Content". So a trick here is to record the original property.
    std::string origProperty_;

    bool inited_;

    bool hasNext_;

    std::vector<TermPositions*> termDocReaders_;

    docid_t currDoc_;

    bool readPositions_;

    std::vector<std::vector<loc_t> > offsets_;

};

typedef PhraseDocumentIterator_<IndexReader> PhraseDocumentIterator;

}

#endif
