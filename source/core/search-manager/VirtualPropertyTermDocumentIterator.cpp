#include "VirtualPropertyTermDocumentIterator.h"
#include "TermDocumentIterator.h"

#include <iostream>

namespace sf1r{

VirtualPropertyTermDocumentIterator::VirtualPropertyTermDocumentIterator(
        const std::string& property)
        :virtualProperty_(property)
{
}

void VirtualPropertyTermDocumentIterator::add(
        TermDocumentIterator* pDocIterator)
{
    if(propIds_.find(pDocIterator->propertyId_) == propIds_.end())
    {
        propIds_.insert(pDocIterator->propertyId_);
        docIteratorList_.push_back((DocumentIterator*)pDocIterator);
    }
}

void VirtualPropertyTermDocumentIterator::doc_item(
        RankDocumentProperty& rankDocumentProperty, 
        unsigned propIndex)
{
    if(propIndex >= docIteratorList_.size()) return;
    DocumentIterator* pEntry = docIteratorList_[propIndex];
    if(pEntry&&pEntry->isCurrent())
        pEntry->doc_item(rankDocumentProperty);	
}

void VirtualPropertyTermDocumentIterator::df_cmtf(
        DocumentFrequencyInProperties& dfmap,
        CollectionTermFrequencyInProperties& ctfmap,
        MaxTermFrequencyInProperties& maxtfmap)
{
    TermDocumentIterator* pEntry;
    std::vector<DocumentIterator*>::iterator iter = docIteratorList_.begin();
    for (; iter != docIteratorList_.end(); ++iter)
    {
        if(*iter)
        {
            pEntry = reinterpret_cast<TermDocumentIterator*>(*iter);
            pEntry->ensureTermDocReader_();
            if (pEntry->pTermDocReader_ == 0)
                continue;
            termid_t termId = pEntry->termId_;

            ID_FREQ_MAP_T& df = dfmap[virtualProperty_];
            df[termId] = df[termId] + (float)pEntry->pTermDocReader_->docFreq();

            ID_FREQ_MAP_T& ctf = ctfmap[virtualProperty_];
            ctf[termId] = ctf[termId] + (float)pEntry->pTermDocReader_->getCTF();

            ID_FREQ_MAP_T& maxtf = maxtfmap[virtualProperty_];
            maxtf[termId] = maxtf[termId] + (float)pEntry->pTermDocReader_->getMaxTF();
        }
    }
}

}
