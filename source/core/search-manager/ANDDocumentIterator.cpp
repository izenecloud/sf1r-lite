#include "ANDDocumentIterator.h"
#include "VirtualPropertyTermDocumentIterator.h"

using namespace std;
using namespace sf1r;

ANDDocumentIterator::ANDDocumentIterator()
    :hasNot_(false)
    ,currDocOfNOTIter_(MAX_DOC_ID)
    ,initNOTIterator_(false)
    ,pNOTDocIterator_(0)
    ,nIteratorNum_(0)
{
}

ANDDocumentIterator::~ANDDocumentIterator()
{
    std::list<DocumentIterator*>::iterator iter = docIterList_.begin();
    for (; iter != docIterList_.end(); ++iter)
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
    else
    {
        docIterList_.push_back(pDocIterator);
        ++nIteratorNum_;
    }
}

void ANDDocumentIterator::add(VirtualPropertyTermDocumentIterator* pDocIterator)
{
    docIterList_.push_back(pDocIterator);
    docIterList_.sort();
    docIterList_.unique();
    nIteratorNum_ = docIterList_.size();
}

void ANDDocumentIterator::df_cmtf(
    DocumentFrequencyInProperties& dfmap,
    CollectionTermFrequencyInProperties& ctfmap,
    MaxTermFrequencyInProperties& maxtfmap)
{
    DocumentIterator* pEntry;
    std::list<DocumentIterator*>::iterator iter = docIterList_.begin();
    for (; iter != docIterList_.end(); ++iter)
    {
        pEntry = (*iter);
        pEntry->df_cmtf(dfmap, ctfmap, maxtfmap);
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
        if (tf < mintf)
        {
            mintf = tf;
        }
    }

    return mintf;
}

void ANDDocumentIterator::queryBoosting(
    double& score,
    double& weight)
{
    DocumentIterator* pEntry;
    std::list<DocumentIterator*>::iterator iter = docIterList_.begin();
    for (; iter != docIterList_.end(); ++iter)
    {
        pEntry = (*iter);
        if (pEntry)
            pEntry->queryBoosting(score, weight);
    }
}

void ANDDocumentIterator::print(int level)
{
    cout << std::string(level*4, ' ') << "|--[ "<< "ANDIter current: " <<current_ <<" "<<  currDoc_ << " ]"<< endl;

    DocumentIterator* pEntry;
    std::list<DocumentIterator*>::iterator iter = docIterList_.begin();
    for (; iter != docIterList_.end(); ++iter)
    {
        pEntry = (*iter);
        if (pEntry)
            pEntry->print(level+1);
    }
}
