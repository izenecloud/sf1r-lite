#include "VirtualTermDocumentIterator.h"
#include "TermDocumentIterator.h"
#include <ir/index_manager/index/TermPositions.h>
#include <ir/index_manager/utility/BitVector.h>

#include <util/profiler/ProfilerGroup.h>

#include <boost/assert.hpp>

using namespace izenelib::ir::indexmanager;

namespace sf1r
{

VirtualTermDocumentIterator::VirtualTermDocumentIterator(
        termid_t termid,
        std::string rawTerm,
        collectionid_t colID,
        izenelib::ir::indexmanager::IndexReader* pIndexReader,
        const std::vector<std::string>& virtualPropreties,
        std::vector<unsigned int>& propertyIdList,
        std::vector<sf1r::PropertyDataType>& dataTypeList,
        unsigned int termIndex,
        bool readPositions
    ):termId_(termid)
    , rawTerm_(rawTerm)
    , colID_(colID)
    , pIndexReader_(pIndexReader)
    , pTermReader_(0)
    , subProperties_(virtualPropreties)
    , propertyIdList_(propertyIdList)
    , dataTypeList_(dataTypeList)
    , termIndex_(termIndex)
    , OrDocIterator_(NULL)
{
    readPositions_ = false;
}

VirtualTermDocumentIterator::~VirtualTermDocumentIterator()
{
    if (OrDocIterator_)
    {
        delete OrDocIterator_;
    }
}

void VirtualTermDocumentIterator::accept()
{
    for (uint32_t i = 0; i < subProperties_.size(); ++i)
    {
        pTermDocReaderList_.push_back(0);
    }
    for (uint32_t i = 0; i < subProperties_.size(); ++i)
    {
        Term term(subProperties_[i].c_str(), termId_);
        if(!pTermReader_)
            pTermReader_ = pIndexReader_->getTermReader(colID_);
        if (!pTermReader_)
            return;
        bool find = pTermReader_->seek(&term);
        if (find)
        {
            df_ += pTermReader_->docFreq(&term);
            if(pTermDocReaderList_[i]) 
            {
                delete pTermDocReaderList_[i];
                pTermDocReaderList_[i] = 0;
            }

            if (readPositions_)
                pTermDocReaderList_[i] = pTermReader_->termPositions();
            else
                pTermDocReaderList_[i] = pTermReader_->termDocFreqs();
            if(!pTermDocReaderList_[i]) find = false;
        }
        else
        {
            delete pTermReader_;
            pTermReader_ = 0;
        }
    }
}

void VirtualTermDocumentIterator::ensureTermDocReader()
{
    for (uint32_t i = 0; i < pTermDocReaderList_.size(); ++i)
    {
        if (pTermDocReaderList_[i] == 0)
            continue;
        if(pTermReader_ == 0)
        {
            pTermReader_ = pIndexReader_->getTermReader(colID_);
            if(pTermReader_ == 0)
                return;
        }
        Term term(subProperties_[i].c_str(), termId_);
        bool find = pTermReader_->seek(&term);
        if (find)
        {
            if (pTermDocReaderList_[i])
            {
                delete pTermDocReaderList_[i];
            }
            if (readPositions_)
                pTermDocReaderList_[i] = pTermReader_->termPositions();
            else
            {
                pTermDocReaderList_[i] = pTermReader_->termDocFreqs();
            }
        }
        else
        {
            //delete pTermReader_;
            //pTermReader_ = 0;
        }
    }
}

void VirtualTermDocumentIterator::doc_item(
    RankDocumentProperty& rankDocumentProperty,
    unsigned propIndex)
{
    unsigned int docLength = 0;
    docid_t docid = doc();
    for (unsigned int i = 0; i < subProperties_.size(); ++i)
    {
        docLength += pIndexReader_->docLength(docid, propertyIdList_[i]);
    }
    rankDocumentProperty.setDocLength(docLength);

    if (readPositions_) 
    {       
            /*
            START_PROFILER ( get_position )
            rankDocumentProperty.activate(termIndex_);
            unsigned int baseLen = 0;
            for (unsigned int i = 0; i < subProperties_.size(); ++i)
            {
                Term term(subProperties_[i].c_str(), termId);
                bool find = pTermReader_->seek(&term);
                pTermDocReader_ = pTermReader_->termPositions();/// only once ..
                
                TermPositions* pPositions = reinterpret_cast<TermPositions*>(pTermDocReader_);
                loc_t pos = pPositions->nextPosition();
                while (pos != BAD_POSITION)
                {
                    rankDocumentProperty.pushPosition(pos + baseLen); // ok
                    pos = pPositions->nextPosition();
                }
                baseLen += pIndexReader_->docLength(doc(), propertyIdList_[i]);
            }*/
    }
    else
    {
        unsigned int freq = 0;
        for (unsigned int i = 0; i < reinterpret_cast<ORDocumentIterator*>(OrDocIterator_)->getdocIteratorList_().size(); ++i)
        {
            sf1r::DocumentIterator* pEntry = reinterpret_cast<ORDocumentIterator*>(OrDocIterator_)->getdocIteratorList_()[i];

            if (pEntry != NULL && pEntry->doc() == docid)
            {
                freq += (reinterpret_cast<TermDocumentIterator*>(pEntry))->tf();
            }
        }
        rankDocumentProperty.setTermFreq(termIndex_, freq);
    }
}

void VirtualTermDocumentIterator::df_cmtf(
    DocumentFrequencyInProperties& dfmap,
    CollectionTermFrequencyInProperties& ctfmap,
    MaxTermFrequencyInProperties& maxtfmap)
{
    //ensureTermDocReader();
    for (unsigned int i = 0; i < pTermDocReaderList_.size(); ++i)
    {
        if (pTermDocReaderList_[i])
        {
            ID_FREQ_MAP_T& df = dfmap[subProperties_[i]];
            df[termId_] = df[termId_] + (float)pTermDocReaderList_[i]->docFreq();

            ID_FREQ_MAP_T& ctf = ctfmap[subProperties_[i]];
            ctf[termId_] = ctf[termId_] + (float)pTermDocReaderList_[i]->getCTF();

            ID_FREQ_MAP_T& maxtf = maxtfmap[subProperties_[i]];
            maxtf[termId_] = maxtf[termId_] + (float)pTermDocReaderList_[i]->getMaxTF();
        }
    }
}

} // namespace sf1r
