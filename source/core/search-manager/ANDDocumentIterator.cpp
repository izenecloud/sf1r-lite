#include "ANDDocumentIterator.h"

using namespace std;
using namespace sf1r;

ANDDocumentIterator::ANDDocumentIterator()
        :hasNot_(false)
        ,currDocOfNOTIter_(MAX_DOC_ID)
        ,initNOTIterator_(false)
        ,pNOTDocIterator_(0)
{
}

ANDDocumentIterator::~ANDDocumentIterator()
{
    for (std::list<DocumentIterator*>::iterator iter = docIterList_.begin(); iter != docIterList_.end(); ++iter)
        delete *iter;
    if (pNOTDocIterator_)
        delete pNOTDocIterator_;
}

void ANDDocumentIterator::add(DocumentIterator* pDocIterator)
{
    if (pDocIterator->isNot())
    {
        hasNot_ = true;
        if (NULL == pNOTDocIterator_)
            pNOTDocIterator_ = new NOTDocumentIterator();
        pNOTDocIterator_->add(pDocIterator);
    }
    else {
        docIterList_.push_back(pDocIterator);
    }
}

bool ANDDocumentIterator::next()
{
    if (!hasNot_)
        return do_next();
    else
    {
        if (docIterList_.empty())
            return false;
        if (! initNOTIterator_)
        {
            initNOTIterator_ = true;
            if (pNOTDocIterator_->next())
                currDocOfNOTIter_ = pNOTDocIterator_->doc();
            else
                currDocOfNOTIter_ = MAX_DOC_ID;
        }

        bool ret = do_next();
        if (currDoc_ == currDocOfNOTIter_) {
            return move_together_with_not();
        }
        else if (currDoc_ < currDocOfNOTIter_)
            return ret;
        else
        {
            currDocOfNOTIter_ = pNOTDocIterator_->skipTo(currDoc_);
            if (currDoc_ == currDocOfNOTIter_)
                return move_together_with_not();
            else
                return ret;
        }
    }
}

#if SKIP_ENABLED
docid_t ANDDocumentIterator::skipTo(docid_t target)
{
    if (!hasNot_)
        return do_skipTo(target);
    else
    {
        docid_t nFoundId, currentDoc = target;
        do
        {
            nFoundId = do_skipTo(currentDoc);
            currDocOfNOTIter_ = pNOTDocIterator_->skipTo(currentDoc);
        }
        while ((nFoundId != MAX_DOC_ID)&&(nFoundId == currDocOfNOTIter_));
        return nFoundId;
    }
}

docid_t ANDDocumentIterator::do_skipTo(docid_t target)
{
    docid_t nFoundId = MAX_DOC_ID;
    size_t nMatch = 0;
    size_t nIteratorNum = docIterList_.size();
    while ( nMatch < nIteratorNum)
    {
        nFoundId = docIterList_.front()->skipTo(target);
        if(MAX_DOC_ID == nFoundId)
            break;
        else if(nFoundId == target)
            nMatch++;
        else if(nFoundId > target)
        {
            target = nFoundId;
            nMatch = 1;
        }
        else break;

        docIterList_.push_back(docIterList_.front());
        docIterList_.pop_front();
    }
    if(nMatch == nIteratorNum)
    {
        currDoc_ = target;
        return nFoundId;
    }
    return MAX_DOC_ID;
}
#endif

void ANDDocumentIterator::doc_item(RankDocumentProperty& rankDocumentProperty)
{
    DocumentIterator* pEntry;
    for (std::list<DocumentIterator*>::iterator iter = docIterList_.begin(); iter != docIterList_.end(); ++iter)
    {
        pEntry = (*iter);
        pEntry->doc_item(rankDocumentProperty);
    }
}

void ANDDocumentIterator::df_ctf(DocumentFrequencyInProperties& dfmap,
        CollectionTermFrequencyInProperties& ctfmap)
{
    DocumentIterator* pEntry;
    std::list<DocumentIterator*>::iterator iter = docIterList_.begin();
    for (; iter != docIterList_.end(); ++iter)
    {
        pEntry = (*iter);
        pEntry->df_ctf(dfmap, ctfmap);
    }
}

count_t ANDDocumentIterator::tf()
{
    DocumentIterator* pEntry;
    count_t mintf = MAX_COUNT;
    count_t tf;
    std::list<DocumentIterator*>::iterator iter = docIterList_.begin();
    for (; iter != docIterList_.end(); ++iter)
    {
        pEntry = (*iter);
        tf = pEntry->tf();
        if (tf < mintf) {
        	mintf = tf;
        }
    }

    return mintf;
}
