/**
 * \file BTreeTermDocIterator.h
 * \brief 
 * \date Aug 9, 2011
 * \author Xin Liu
 */

#ifndef BTREE_TERM_DOC_ITERATOR_H_
#define BTREE_TERM_DOC_ITERATOR_H_

#include "DocumentIterator.h"
#include <common/PropertyValue.h>
#include <index-manager/IndexManager.h>

#include <ir/index_manager/utility/BitMapIterator.h>
#include <boost/assert.hpp>

#include <vector>
#include <string>

namespace sf1r{

///DocumentIterator for one term in numeric filtering property
class BTreeTermDocIterator: public DocumentIterator
{
public:
    BTreeTermDocIterator(
        termid_t termId,
        const std::string& rawTerm,
        collectionid_t colID,
        izenelib::ir::indexmanager::IndexReader* pIndexReader,
        const std::string& property,
        unsigned int propertyId,
        sf1r::PropertyDataType propertyDataType,
        boost::shared_ptr<IndexManager> indexManagerPtr
    );

    ~BTreeTermDocIterator();

public:
    void add(DocumentIterator* pDocIterator){}

    void ensure_accept(izenelib::ir::indexmanager::TermDocFreqs* pTermDocReader)
    { pBTreeTermDocReader_ = (izenelib::ir::indexmanager::BitMapIterator*)pTermDocReader; }

    bool accept();

    void set(izenelib::ir::indexmanager::TermDocFreqs* pTermDocReader)
    {
        std::cout<<"***********enter BTreeTermDocIterator:set function*************"<<std::endl;
        pBTreeTermDocReader_ = (izenelib::ir::indexmanager::BitMapIterator*)pTermDocReader;
        df_ = pBTreeTermDocReader_->docFreq();
        destroy_ = true;
        std::cout<<"************rawTerm df is****************"<<rawTerm_<<", "<<df_<<std::endl;
    }

    bool next()
    {
        if (pBTreeTermDocReader_)
            return pBTreeTermDocReader_->next();
    return false;
    }

    docid_t doc()
    {
        BOOST_ASSERT(pBTreeTermDocReader_);
        return pBTreeTermDocReader_->doc();
    }

    void doc_item(RankDocumentProperty& rankDocumentProperty){}

    void df_ctf(DocumentFrequencyInProperties& dfmap, CollectionTermFrequencyInProperties& ctfmap){};

#if SKIP_ENABLED
    docid_t skipTo(docid_t target)
    {
        return pBTreeTermDocReader_->skipTo(target);
    }
#endif

    unsigned int df() {return df_;}

    void set_df(unsigned int df) {df_ = df;}

    count_t tf()
    {
        BOOST_ASSERT(pBTreeTermDocReader_);
        return pBTreeTermDocReader_->freq();
    }

    termid_t termId() {return termId_;}

    void print(int level=0)
    {
        cout << std::string(level*4, ' ') << "|--[ "
                << "TermIter " << current_
                << " - termid: " << termId_ << " " << property_<<" ]"<< endl;
    }

protected:
    termid_t termId_;

    std::string rawTerm_;

    collectionid_t colID_;

    izenelib::ir::indexmanager::IndexReader* pIndexReader_;

    std::string property_;

    unsigned int propertyId_;

    PropertyDataType propertyDataType_;

    izenelib::ir::indexmanager::BitMapIterator* pBTreeTermDocReader_;

    boost::shared_ptr<IndexManager> indexManagerPtr_;

    unsigned int df_;

    bool destroy_;
};

}

#endif /* BTREE_TERM_DOC_ITERATOR_H_ */
