#include "WildcardDocumentIterator.h"

using namespace izenelib::ir::indexmanager;

namespace sf1r {

WildcardDocumentIterator::WildcardDocumentIterator(
    collectionid_t colID,
    IndexReader* pIndexReader,
    const std::string& property,
    unsigned int propertyId,
    bool readPositions,
    int maxTerms)
        :maxTermThreshold_(maxTerms)
        ,pWildcardDocIteratorQueue_(NULL)
        ,colID_(colID)
        ,pIndexReader_(pIndexReader)
        ,pTermReader_(NULL)
        ,property_(property)
        ,propertyId_(propertyId)
        ,readPositions_(readPositions)

{
    pTermReader_ = pIndexReader_->getTermReader(colID);

}

WildcardDocumentIterator::~WildcardDocumentIterator()
{
    if (pWildcardDocIteratorQueue_)
        delete pWildcardDocIteratorQueue_;
    if (pTermReader_)
    {
        delete pTermReader_;
        pTermReader_ = NULL;
    }
}

void WildcardDocumentIterator::getTermIds(std::vector<termid_t>& termIds)
{
    for (size_t i = 0; i < pWildcardDocIteratorQueue_->size(); ++i)
    {
        TermDocumentIterator* pDocIterator = pWildcardDocIteratorQueue_->getAt(i);
        termIds.push_back(pDocIterator->termId());
    }
}

void WildcardDocumentIterator::add(
        termid_t termId,
        unsigned termIndex,
        std::map<termid_t, std::vector<izenelib::ir::indexmanager::TermDocFreqs*> >& termDocReaders)
{
#if PREFETCH_TERMID
    std::map<termid_t, std::vector<izenelib::ir::indexmanager::TermDocFreqs*> >::iterator constIt
				 = termDocReaders.find(termId);
    if(constIt != termDocReaders.end())
    {
        TermDocumentIterator* pIterator =
            new TermDocumentIterator(termId,
                                     colID_,
                                     pIndexReader_,
                                     property_,
                                     propertyId_,
                                     termIndex,
                                     readPositions_);
        pIterator->set(constIt->second.back() );
        add(pIterator);
        constIt->second.pop_back();
    }
    else
#endif
    {
        size_t df = 0;
        Term term(property_.c_str(),termId);

        bool find = pTermReader_->seek(&term);

        if (find)
        {
            df = pTermReader_->docFreq(&term);
        }
        if (df > 0)
        {
            TermDocumentIterator* pIterator =
                new TermDocumentIterator(termId,
                                         colID_,
                                         pIndexReader_,
                                         property_,
                                         propertyId_,
                                         termIndex,
                                         readPositions_);
            pIterator->set_df(df);
            add(pIterator);
        }
    }
}

void WildcardDocumentIterator::add(TermDocumentIterator* pDocIterator)
{
    if (NULL == pWildcardDocIteratorQueue_)
        pWildcardDocIteratorQueue_ = new WildcardDocumentIteratorQueue(maxTermThreshold_);
    pWildcardDocIteratorQueue_->insert(pDocIterator);
}

void WildcardDocumentIterator::initDocIteratorQueue()
{
    if (pWildcardDocIteratorQueue_->size() < 1)
        return;

    if (pTermReader_)
    {
        delete pTermReader_;
        pTermReader_ = NULL;
    }

    pDocIteratorQueue_ = new DocumentIteratorQueue(pWildcardDocIteratorQueue_->size());
    for (size_t i = 0; i < pWildcardDocIteratorQueue_->size(); ++i)
    {
        DocumentIterator* pDocIterator = pWildcardDocIteratorQueue_->getAt(i);
        if (pDocIterator->next())
        {
            docIteratorList_.push_back(pDocIterator);
            pDocIteratorQueue_->insert(pDocIterator);
        }
    }

    pWildcardDocIteratorQueue_->setDel(false);
}

bool WildcardDocumentIterator::next()
{
    return do_next();
}

void WildcardDocumentIterator::df_ctf(DocumentFrequencyInProperties& dfmap,
        CollectionTermFrequencyInProperties& ctfmap)
{
    for (size_t i = 0; i < pWildcardDocIteratorQueue_->size(); ++i)
    {
        DocumentIterator* pDocIterator = pWildcardDocIteratorQueue_->getAt(i);
        pDocIterator->df_ctf(dfmap, ctfmap);
    }
}

unsigned int WildcardDocumentIterator::numIterators()
{
    if (!pWildcardDocIteratorQueue_)
        return 0;
    else
        return pWildcardDocIteratorQueue_->size();
}

} // namespace sf1r
