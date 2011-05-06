/**
 * @file sf1r/search-manager/TermDocumentIterator.h
 * @author Yingfeng Zhang
 * @date Created <2009-09-22>
 * @brief DocumentIterator which iterate posting for one term
 */
#ifndef TERM_DOCUMENT_ITERATOR_H
#define TERM_DOCUMENT_ITERATOR_H

#include "DocumentIterator.h"

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

    ~TermDocumentIterator();

public:
    void add(DocumentIterator* pDocIterator){}

    void ensure_accept(izenelib::ir::indexmanager::TermDocFreqs* pTermDocReader)
    { pTermDocReader_ = pTermDocReader; }

    bool accept();

    void set(izenelib::ir::indexmanager::TermDocFreqs* pTermDocReader)
    {
        pTermDocReader_ = pTermDocReader;
        df_ = pTermDocReader_->docFreq();
        destroy_ = true;
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

    void doc_item(RankDocumentProperty& rankDocumentProperty);

    unsigned int df() {return df_;}

    void set_df(unsigned int df) {df_ = df;}

    void df_ctf(DocumentFrequencyInProperties& dfmap, CollectionTermFrequencyInProperties& ctfmap);

    count_t tf()
    {
    	BOOST_ASSERT(pTermDocReader_);
    	return pTermDocReader_->freq();
    }

    termid_t termId() {return termId_;}

    void print(int level=0)
    {
        cout << std::string(level*4, ' ') << "|--[ "
                << "TermIter " << current_
                << " - termid: " << termId_ << " " << property_<<" ]"<< endl;
    }

private:
    termid_t termId_;

    collectionid_t colID_;

    std::string property_;

    unsigned int propertyId_;

    unsigned int termIndex_;

    izenelib::ir::indexmanager::IndexReader* pIndexReader_;

    izenelib::ir::indexmanager::TermReader* pTermReader_;

    izenelib::ir::indexmanager::TermDocFreqs* pTermDocReader_;

    docid_t currDoc_;

    unsigned int df_;

    bool readPositions_;

    bool destroy_;
};

}

#endif
