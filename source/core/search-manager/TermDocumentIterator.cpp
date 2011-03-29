#include "TermDocumentIterator.h"

#include <ir/index_manager/index/TermPositions.h>

#include <util/profiler/ProfilerGroup.h>

#include <boost/assert.hpp>

using namespace izenelib::ir::indexmanager;

namespace sf1r {

TermDocumentIterator::TermDocumentIterator(
    termid_t termid,
    collectionid_t colID,
    IndexReader* pIndexReader,
    const std::string& property,
    unsigned int propertyId,
    unsigned int termIndex,
    bool readPositions
)
        :termId_(termid)
        ,colID_(colID)
        ,property_(property)
        ,propertyId_(propertyId)
        ,termIndex_(termIndex)
        ,pIndexReader_(pIndexReader)
        ,pTermReader_(0)
        ,pTermDocReader_(0)
        ,df_(0)
        ,readPositions_(readPositions)
        ,destroy_(true)
{
}

TermDocumentIterator::~TermDocumentIterator()
{
    if(destroy_)
        if(pTermDocReader_)
            delete pTermDocReader_;
    if(pTermReader_)
        delete pTermReader_;
}

bool TermDocumentIterator::accept()
{
    Term term(property_.c_str(),termId_);

    pTermReader_ = pIndexReader_->getTermReader(colID_);

    if (!pTermReader_)
        return false;
//    if(pIndexReader_->isDirty())
//    {
//        if (pTermReader_) delete pTermReader_;
//        pTermReader_ = 0;
//        return false;
//    }
    bool find = pTermReader_->seek(&term);

//    if(pIndexReader_->isDirty())
//    {
//        if (pTermReader_) delete pTermReader_;
//        pTermReader_ = 0;
//        return false;
//    }
    if (find)
    {
        df_ = pTermReader_->docFreq(&term);
        //entry->pPositions = 0;//pPositions;
        if (readPositions_)
            pTermDocReader_ = pTermReader_->termPositions();
        else
            pTermDocReader_ = pTermReader_->termDocFreqs();
    }
    else
    {
        delete pTermReader_;
        pTermReader_ = 0;
    }

    return find;
}

void TermDocumentIterator::doc_item(RankDocumentProperty& rankDocumentProperty)
{
    CREATE_PROFILER ( get_position, "SearchManager", "doSearch_: getting positions");

    if (readPositions_)
    BOOST_ASSERT(termIndex_ < rankDocumentProperty.size());

    if (rankDocumentProperty.termFreqAt(termIndex_) == 0)
    {
        rankDocumentProperty.setDocLength(
            pIndexReader_->docLength(doc(), propertyId_)
        );

        if (readPositions_)
        {
            START_PROFILER ( get_position )

            rankDocumentProperty.activate(termIndex_);

            TermPositions* pPositions = (TermPositions*)pTermDocReader_;
            loc_t pos = pPositions->nextPosition();
            while (pos != BAD_POSITION)
            {
                rankDocumentProperty.pushPosition(pos);
                pos = pPositions->nextPosition();
            }
            STOP_PROFILER ( get_position )
        }
        else
        {
            rankDocumentProperty.setTermFreq(termIndex_, pTermDocReader_->freq());
        }
    }
}


void TermDocumentIterator::df_ctf(DocumentFrequencyInProperties& dfmap, CollectionTermFrequencyInProperties& ctfmap)
{
    if (pTermDocReader_ == 0)
    {
        if(pTermReader_ == 0)
        {
            pTermReader_ = pIndexReader_->getTermReader(colID_);
            Term term(property_.c_str(),termId_);
            pTermReader_->seek(&term);
            if(pTermReader_ == 0)
                return;
        }

        if (readPositions_)
            pTermDocReader_ = pTermReader_->termPositions();
        else
            pTermDocReader_ = pTermReader_->termDocFreqs();
    }
    if (pTermDocReader_ == 0)
        return;
    ID_FREQ_MAP_T& df = dfmap[property_];
    df[termId_] = df[termId_] + (float)pTermDocReader_->docFreq();

    ID_FREQ_MAP_T& ctf = ctfmap[property_];
    ctf[termId_] = ctf[termId_] + (float)pTermDocReader_->getCTF();
}

} // namespace sf1r
