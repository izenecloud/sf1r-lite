#include "TermDocumentIterator.h"

#include <ir/index_manager/index/TermPositions.h>
#include <ir/index_manager/utility/BitVector.h>

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
        ,rawTerm_("")
        ,colID_(colID)
        ,property_(property)
        ,propertyId_(propertyId)
        ,dataType_(sf1r::STRING_PROPERTY_TYPE)
        ,isNumericFilter_(false)
        ,termIndex_(termIndex)
        ,pIndexReader_(pIndexReader)
        ,pTermReader_(0)
        ,pTermDocReader_(0)
        ,df_(0)
        ,readPositions_(readPositions)
        ,destroy_(true)
{
}

TermDocumentIterator::TermDocumentIterator(
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
)
        :termId_(termid)
        ,rawTerm_(rawTerm)
        ,colID_(colID)
        ,property_(property)
        ,propertyId_(propertyId)
        ,dataType_(dataType)
        ,isNumericFilter_(isNumericFilter)
        ,termIndex_(termIndex)
        ,pIndexReader_(pIndexReader)
        ,pTermReader_(0)
        ,pTermDocReader_(0)
        ,indexManagerPtr_(indexManagerPtr)
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
    if(!isNumericFilter_)
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
    else
    {
        boost::shared_ptr<EWAHBoolArray<uint32_t> > pDocIdSet;
        boost::shared_ptr<BitVector> pBitVector;
        if (!TermTypeDetector::isTypeMatch(rawTerm_, dataType_))
            return false;

        PropertyType value;
        PropertyValue2IndexPropertyType converter(value);
        boost::apply_visitor(converter, TermTypeDetector::propertyValue_.getVariant());

        bool find = indexManagerPtr_->seekTermFromBTreeIndex(colID_, property_, value);
        if (find)
        {
             pDocIdSet.reset(new EWAHBoolArray<uint32_t>());
             pBitVector.reset(new BitVector(pIndexReader_->numDocs() + 1));

             indexManagerPtr_->getDocsByNumericValue(colID_, property_, value, *pBitVector);
             pBitVector->compressed(*pDocIdSet);
             pTermDocReader_ = (TermDocFreqs*)(new EWAHBoolArrayBitIterator<uint32_t>(pDocIdSet->bit_iterator()));
             df_ = pTermDocReader_->docFreq();
         }
        return find;
    }
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

        TermPositions* pPositions = dynamic_cast<TermPositions*>(pTermDocReader_);
        if (readPositions_ && pPositions)
        {
            START_PROFILER ( get_position )

            rankDocumentProperty.activate(termIndex_);

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
