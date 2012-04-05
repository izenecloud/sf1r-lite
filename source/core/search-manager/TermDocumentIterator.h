/**
 * @file sf1r/search-manager/TermDocumentIterator.h
 * @author Yingfeng Zhang
 * @date Created <2009-09-22>
 * @brief DocumentIterator which iterate posting for one term
 */
#ifndef TERM_DOCUMENT_ITERATOR_H
#define TERM_DOCUMENT_ITERATOR_H

#include "DocumentIterator.h"
#include <common/TermTypeDetector.h>
#include <index-manager/IndexManager.h>

#include <ir/index_manager/index/AbsTermReader.h>
#include <ir/index_manager/index/IndexReader.h>
#include <ir/index_manager/index/TermDocFreqs.h>
#include <ir/index_manager/utility/PriorityQueue.h>

#include <boost/assert.hpp>

#include <vector>
#include <string>

namespace sf1r{

///DocumentIterator for one term in one property
class TermDocumentIterator: public DocumentIterator
{
public:
    TermDocumentIterator(
            termid_t termid,
            collectionid_t colID,
            izenelib::ir::indexmanager::IndexReader* pIndexReader,
            const std::string& property,
            unsigned int propertyId,
            unsigned int termIndex,
            bool readPositions
    );

    TermDocumentIterator(
            termid_t termid,
            std::string rawTerm,
            collectionid_t colID,
            izenelib::ir::indexmanager::IndexReader* pIndexReader,
            boost::shared_ptr<IndexManager> indexManagerPtr,
            const std::string& property,
            unsigned int propertyId,
            sf1r::PropertyDataType dataType,
            bool isNumericFilter,
            unsigned int termIndex,
            bool readPositions
    );

    ~TermDocumentIterator();

public:
    void add(DocumentIterator* pDocIterator){}

    bool accept();

    void set(
        izenelib::ir::indexmanager::TermDocFreqs* pTermDocReader)
    {
        if(pTermDocReader_) delete pTermDocReader_;
        pTermDocReader_ = pTermDocReader;
        df_ = pTermDocReader_->docFreq();
    }

    bool next()
    {
        if (pTermDocReader_)
            return pTermDocReader_->next();
        return false;
    }

    docid_t doc()
    {
        BOOST_ASSERT(pTermDocReader_);
        return pTermDocReader_->doc();
    }

#if SKIP_ENABLED
    docid_t skipTo(docid_t target)
    {
        return pTermDocReader_->skipTo(target);
    }
#endif

    void doc_item(RankDocumentProperty& rankDocumentProperty, unsigned propIndex = 0);

    unsigned int df() {return df_;}

    void set_df(unsigned int df) {df_ = df;}

    void df_cmtf(
        DocumentFrequencyInProperties& dfmap,
        CollectionTermFrequencyInProperties& ctfmap,
        MaxTermFrequencyInProperties& maxtfmap);

    count_t tf()
    {
        BOOST_ASSERT(pTermDocReader_);
        return pTermDocReader_->freq();
    }

    void set_ub(float ub){
        ub_ = ub;
    }

    termid_t termId() {return termId_;}

    unsigned int termIndex() {return termIndex_;}

    void print(int level=0)
    {
        cout << std::string(level*4, ' ') << "|--[ "
                << "TermIter " << current_
                << " - termid: " << termId_ << " " << property_<<" ]"<< endl;
    }

private:
    TermDocumentIterator(const TermDocumentIterator&);
    void operator=(const TermDocumentIterator&);

    void ensureTermDocReader_();

protected:
    termid_t termId_;

    std::string rawTerm_;

    collectionid_t colID_;

    std::string property_;

    unsigned int propertyId_;

    sf1r::PropertyDataType dataType_;

    bool isNumericFilter_;

    unsigned int termIndex_;

    izenelib::ir::indexmanager::IndexReader* pIndexReader_;

    izenelib::ir::indexmanager::TermReader* pTermReader_;

    izenelib::ir::indexmanager::TermDocFreqs* pTermDocReader_;

    boost::shared_ptr<IndexManager> indexManagerPtr_;

    docid_t currDoc_;

    unsigned int df_;

    bool readPositions_;

    float ub_;
    friend class WANDDocumentIterator;
    friend class VirtualPropertyTermDocumentIterator;
};

}

#endif
