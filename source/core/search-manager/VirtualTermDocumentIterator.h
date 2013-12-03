/**
 * @file sf1r/search-manager/VirtualTermDocumentIterator.h
 * @author Hongliang Zhao
 * @date Created <2012-12-18>
 * @brief DocumentIterator which iterate posting for one term
 */

#ifndef VIRTUAL_TERM_DOCUMENT_ITERATOR_H
#define VIRTUAL_TERM_DOCUMENT_ITERATOR_H

#include "DocumentIterator.h"
#include "ORDocumentIterator.h"

#include <common/TermTypeDetector.h>

#include <ir/index_manager/index/AbsTermReader.h>
#include <ir/index_manager/index/IndexReader.h>
#include <ir/index_manager/index/TermDocFreqs.h>
#include <ir/index_manager/utility/PriorityQueue.h>

#include <boost/assert.hpp>

#include <vector>
#include <string>

namespace sf1r
{

class InvertedIndexManager;

///DocumentIterator for one term in one virtual property
class VirtualTermDocumentIterator: public DocumentIterator
{
public:
    VirtualTermDocumentIterator(
        termid_t termid,
        std::string rawTerm,
        collectionid_t colID,
        izenelib::ir::indexmanager::IndexReader* pIndexReader,
        const std::vector<std::string>& virtualPropreties,
        std::vector<unsigned int>& propertyIdList,
        std::vector<sf1r::PropertyDataType>& dataTypeList,
        unsigned int termIndex,
        bool readPositions
    );

    ~VirtualTermDocumentIterator();

public:
    void add(DocumentIterator* pDocIterator) 
    {
        OrDocIterator_ = pDocIterator;
    }

    void add(VirtualPropertyTermDocumentIterator* pDocIterator) {};

    void set(
        izenelib::ir::indexmanager::TermDocFreqs* pTermDocReader)
    {
        pTermDocReaderList_.push_back(pTermDocReader);
        if (pTermDocReader)
        {
            df_ += pTermDocReader->docFreq();
        }
    }

    void accept();

    bool next()
    {
        return OrDocIterator_->next();
    }

    docid_t doc()
    {
        return OrDocIterator_->doc();
    }

    bool isCurrent()
    {
        return OrDocIterator_->isCurrent();
    }

#if SKIP_ENABLED
    docid_t skipTo(docid_t target)
    {
        return OrDocIterator_->skipTo(target);
    }
#endif

    void doc_item(RankDocumentProperty& rankDocumentProperty, unsigned propIndex = 0);

    unsigned int df()
    {
        return df_;
    }

    void set_df(unsigned int df)
    {
        df_ = df;
    }

    void df_cmtf(
        DocumentFrequencyInProperties& dfmap,
        CollectionTermFrequencyInProperties& ctfmap,
        MaxTermFrequencyInProperties& maxtfmap);

    count_t tf()
    {
        count_t freq = 0;
        for (uint32_t i = 0; i < pTermDocReaderList_.size(); ++i)
        {
            if (pTermDocReaderList_[i])
            {
                freq += pTermDocReaderList_[i]->freq();
            }
        }
        return freq;
    }

    void set_ub(float ub)
    {
        ub_ = ub;
    }

    termid_t termId()
    {
        return termId_;
    }

    unsigned int termIndex()
    {
        return termIndex_;
    }

    void print(int level=0)
    {
        cout << std::string(level*4, ' ') << "|--[ "
             << " - termid: " << termId_ << " " <<" ]"<< endl
             << " - df :" << df_ <<endl
             << " - tf :" << tf() <<endl;
    }
    
    void queryBoosting(double& score, double& weight) {}

    double score(
        const RankQueryProperty& rankQueryProperty,
        const boost::shared_ptr<PropertyRanker>& propertyRanker
    )
    {
        return 0.0f;
    }

    double score(
        const std::vector<RankQueryProperty>& rankQueryProperties,
        const std::vector<boost::shared_ptr<PropertyRanker> >& propertyRankers
    )
    {
        return 0.0f;
    }

private:
    void operator=(const VirtualTermDocumentIterator&);
    void ensureTermDocReader();

protected:
    termid_t termId_;

    std::string rawTerm_;

    collectionid_t colID_;

    bool isNumericFilter_;

    izenelib::ir::indexmanager::IndexReader* pIndexReader_;

    izenelib::ir::indexmanager::TermReader* pTermReader_;

    std::vector<izenelib::ir::indexmanager::TermDocFreqs*> pTermDocReaderList_;

    const std::vector<std::string> subProperties_;
 
    std::vector<unsigned int> propertyIdList_;

    std::vector<sf1r::PropertyDataType> dataTypeList_;

    unsigned int termIndex_;

    docid_t currDoc_;

    unsigned int df_;

    bool readPositions_;

    float ub_;

    DocumentIterator* OrDocIterator_;

    friend class WANDDocumentIterator;
    friend class VirtualPropertyTermDocumentIterator;
};

}

#endif
