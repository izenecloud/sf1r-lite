#include "ORDocumentIterator.h"
#include "NOTDocumentIterator.h"

using namespace std;
using namespace sf1r;

ORDocumentIterator::ORDocumentIterator()
        :pDocIteratorQueue_(NULL)
        ,hasNot_(false)
        ,currDocOfNOTIter_(MAX_DOC_ID)
        ,initNOTIterator_(false)
        ,pNOTDocIterator_(NULL)
{
}

ORDocumentIterator::~ORDocumentIterator()
{
    vector<DocumentIterator*>::iterator iter = docIteratorList_.begin();
    for (; iter != docIteratorList_.end(); ++iter)
        if(*iter) delete *iter;

    if (pNOTDocIterator_)
        delete pNOTDocIterator_;

    if (pDocIteratorQueue_)
        delete pDocIteratorQueue_;
}

void ORDocumentIterator::initDocIteratorQueue()
{
    if (docIteratorList_.size() < 1)
        return;
    pDocIteratorQueue_ = new DocumentIteratorQueue(docIteratorList_.size());
    DocumentIterator* pDocIterator;
    vector<DocumentIterator*>::iterator iter = docIteratorList_.begin();
    while (iter != docIteratorList_.end())
    {
        pDocIterator = (*iter);
        if (pDocIterator)
        {
            pDocIterator->setCurrent(false);
            if(pDocIterator->next())
                pDocIteratorQueue_->insert(pDocIterator);
           else
           {
               *iter = NULL;
               delete pDocIterator;
           }
        }
        iter ++;
    }
}

void ORDocumentIterator::add(DocumentIterator* pDocIterator)
{
    if (pDocIterator->isNot())
    {
        hasNot_ = true;
        if (NULL == pNOTDocIterator_)
            pNOTDocIterator_ = new NOTDocumentIterator();
        pNOTDocIterator_->add(pDocIterator);
    }
    else
        docIteratorList_.push_back(pDocIterator);
}

bool ORDocumentIterator::next()
{
    if (!hasNot_)
        return do_next();
    else
    {
        if (! initNOTIterator_)
        {
            initNOTIterator_ = true;
            if (pNOTDocIterator_->next())
                currDocOfNOTIter_ = pNOTDocIterator_->doc();
            else
                currDocOfNOTIter_ = MAX_DOC_ID;
        }

        bool ret = do_next();

        if (currDoc_ == currDocOfNOTIter_)
            return move_together_with_not();
        if (currDoc_ < currDocOfNOTIter_)
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

bool ORDocumentIterator::move_together_with_not()
{
    bool ret;
    do
    {
        ret = do_next();
        if (pNOTDocIterator_->next())
            currDocOfNOTIter_ = pNOTDocIterator_->doc();
        else
            currDocOfNOTIter_ = MAX_DOC_ID;
    }
    while (ret&&(currDoc_ == currDocOfNOTIter_));

    return ret;
}

bool ORDocumentIterator::do_next(){
    if(currDoc_ == MAX_DOC_ID){
        for(std::vector<DocumentIterator*>::iterator iter = docIteratorList_.begin();
                iter != docIteratorList_.end(); ++iter){
            DocumentIterator* pEntry = (*iter);
            while(pEntry -> next)
                doc_set.set(pEntry ->  doc());
        }
        for(docid_t docid = currDoc_ + 1; docid < doc_set.size(); ++docid){
            if(doc_set.test(docid){
                currDoc_ = docid;
                return true;
            }
        }
        return false;
}

bool ORDocumentIterator::do_next_bk()
{
    if (pDocIteratorQueue_ == NULL)
    {
        initDocIteratorQueue();

        if (pDocIteratorQueue_ == NULL)
            return false;

        DocumentIterator* top = pDocIteratorQueue_->top();
        if (top == NULL)
            return false;

        currDoc_ = top->doc();

        for (size_t i = 0; i < pDocIteratorQueue_->size(); ++i)
        {
            DocumentIterator* pEntry = pDocIteratorQueue_->getAt(i);
            if (currDoc_ == pEntry->doc())
                pEntry->setCurrent(true);
            else
                pEntry->setCurrent(false);
        }
        return true;
    }


    DocumentIterator* top = pDocIteratorQueue_->top();

    while (top != NULL && top->isCurrent())
    {
        top->setCurrent(false);
        if (top->next())
            pDocIteratorQueue_->adjustTop();
        else pDocIteratorQueue_->pop();
        top = pDocIteratorQueue_->top();
    }

    if (top == NULL)
        return false;

    currDoc_ = top->doc();

    //for (size_t i = 0; i < pDocIteratorQueue_->size(); ++i)
    ///we can not use priority queue here because if some dociterator
    ///is removed from that queue, we should set flag for it.
    for (std::vector<DocumentIterator*>::iterator iter = docIteratorList_.begin();
      iter != docIteratorList_.end(); ++iter)
    {
        //DocumentIterator* pEntry = pDocIteratorQueue_->getAt(i);
        DocumentIterator* pEntry = (*iter);
        if(pEntry)
        {
            if (currDoc_ == pEntry->doc())
                pEntry->setCurrent(true);
            else
                pEntry->setCurrent(false);
        }
    }

    return true;
}

#if SKIP_ENABLED
docid_t ORDocumentIterator::skipTo(docid_t target)
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

docid_t ORDocumentIterator::do_skipTo(docid_t target)
{
    if(pDocIteratorQueue_ == NULL)
    {
        initDocIteratorQueue();
        if (pDocIteratorQueue_ == NULL)
            return MAX_DOC_ID;
    }

    if (pDocIteratorQueue_->size() < 1)
    {
        return MAX_DOC_ID;
    }
    docid_t nFoundId = MAX_DOC_ID;
    do
    {
        DocumentIterator* top = pDocIteratorQueue_->top();
        currDoc_ = top->doc();

        std::vector<DocumentIterator*>::iterator iter = docIteratorList_.begin();
        for (; iter != docIteratorList_.end(); ++iter)
        {
            if (*iter)
            {
                if((*iter)->doc() == currDoc_)
                    (*iter)->setCurrent(true);
                else
                    (*iter)->setCurrent(false);
            }
        }

        if (currDoc_ >= target)
        {
            if(pDocIteratorQueue_->size() < 1)
            {
                return MAX_DOC_ID;
            }
            else
            {
                return currDoc_;
            }
        }
        else
        {
            nFoundId = top->skipTo(target);

            if((MAX_DOC_ID == nFoundId)||(nFoundId < target))
            {
                pDocIteratorQueue_->pop();
                if (pDocIteratorQueue_->size() < 1)
                {
                    return MAX_DOC_ID;
                }
            }
            else
            {
                pDocIteratorQueue_->adjustTop();
            }
        }
    } while (true);

}
#endif

void ORDocumentIterator::doc_item(
    RankDocumentProperty& rankDocumentProperty,
    unsigned propIndex)
{
    DocumentIterator* pEntry;
    for (size_t i = 0; i < pDocIteratorQueue_->size(); ++i)
    {
        pEntry = pDocIteratorQueue_->getAt(i);
        if (pEntry->isCurrent())
            pEntry->doc_item(rankDocumentProperty,propIndex);
    }
}

void ORDocumentIterator::df_cmtf(
    DocumentFrequencyInProperties& dfmap,
    CollectionTermFrequencyInProperties& ctfmap,
    MaxTermFrequencyInProperties& maxtfmap)
{
    DocumentIterator* pEntry;
    std::vector<DocumentIterator*>::iterator iter = docIteratorList_.begin();
    for (; iter != docIteratorList_.end(); ++iter)
    {
        pEntry = (*iter);
        if(pEntry)
            pEntry->df_cmtf(dfmap, ctfmap, maxtfmap);
    }
}

count_t ORDocumentIterator::tf()
{
    DocumentIterator* pEntry;
    count_t maxtf = 0;
    count_t tf=0;
    for (size_t i = 0; i < pDocIteratorQueue_->size(); ++i)
    {
        pEntry = pDocIteratorQueue_->getAt(i);
        if (pEntry->isCurrent())
        {
            tf = pEntry->tf();
            if (tf > maxtf)
                maxtf = tf;
        }
    }


    return maxtf;
}

void ORDocumentIterator::queryBoosting(
    double& score,
    double& weight)
{
    DocumentIterator* pEntry;
    for (size_t i = 0; i < pDocIteratorQueue_->size(); ++i)
    {
        pEntry = pDocIteratorQueue_->getAt(i);
        if (pEntry->isCurrent())
        {
            pEntry->queryBoosting(score, weight);
        }
    }
}

void ORDocumentIterator::print(int level)
{
    cout << std::string(level*4, ' ') << "|--[ "<< "ORIter current: " << current_<<" "<< currDoc_ << " ]"<< endl;

    DocumentIterator* pEntry;
    for (size_t i = 0; i < docIteratorList_.size(); ++i)
    {
        pEntry = docIteratorList_[i];
        if (pEntry)
            pEntry->print(level+1);
    }
}
