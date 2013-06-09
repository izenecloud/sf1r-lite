/**
 * \file WANDDocumentIterator.cpp
 * \brief
 * \date Feb 29, 2012
 * \author Xin Liu
 * \brief:
 * \1) One WANDDocumentIterator for only one property.
 * \2) Query items whose UB is 0 make contribution to threshold.
 * \update May 29, 2013
 * \author Kevin Lin
 */

#include "WANDDocumentIterator.h"
#include <common/type_defs.h>
#include <util/get.h>

namespace sf1r
{

WANDDocumentIterator::WANDDocumentIterator()
{
    currThreshold_ = 0.0;
    currDoc_   = 0;
    pivotDoc_  = 0;
}

WANDDocumentIterator::~WANDDocumentIterator()
{
    std::vector<DocumentIterator*>::iterator iter = docIteratorList_.begin();
    for (; iter != docIteratorList_.end(); ++iter)
    {
        if(*iter)
            delete *iter;
    }
}

void WANDDocumentIterator::add(DocumentIterator* pDocIterator)
{
    boost::mutex::scoped_lock lock(mutex_);
    docIteratorList_.push_back(pDocIterator);
}

void WANDDocumentIterator::setUB(bool useOriginalQuery, UpperBoundInProperties& ubmap)
{
    if (docIteratorList_.begin() == docIteratorList_.end())
        return;
    
    std::vector<DocumentIterator*>::iterator iter = docIteratorList_.begin();
    for( ; iter != docIteratorList_.end(); ++iter )
    {
        DocumentIterator* pEntry = (*iter);
        if (pEntry)
        {
            pEntry->setUB(useOriginalQuery, ubmap);
        }
    }

    unsigned short nItems = docIteratorList_.size();
    //LOG(INFO)<<"==nTotal=="<<missRate_<<"==nItems=="<<nItems;
    missRate_ = (missRate_ - nItems) / missRate_;
    //LOG(INFO)<<"====WAND::missRate===>>"<<missRate_;
    //LOG(INFO)<<"====WAND::getUB===>>"<<getUB();
    
}

float WANDDocumentIterator::getUB()
{
    float sumUB = 0.0;
    std::vector<DocumentIterator*>::iterator it = docIteratorList_.begin();
    for (; it != docIteratorList_.end(); it++)
    {
        if (!*it)
            continue;
        sumUB += (*it)->getUB();
    }
    return sumUB / (1 - missRate_);
}

void WANDDocumentIterator::initThreshold(float threshold)
{
    std::vector<DocumentIterator*>::iterator it = docIteratorList_.begin();
    for (; it != docIteratorList_.end(); it++)
    {
        (*it)->initThreshold(threshold);
    }
    float sumUBs = getUB();
    if(threshold > 0.0F)
    {
        currThreshold_ = sumUBs * threshold;
    }
    else
    {
        currThreshold_ = sumUBs * 0.5;
    }

    //LOG(INFO)<<"the initial currThreshold = "<<currThreshold_<<"for property:  "<<(*docIteratorList_.begin())->getProperty();
}

void WANDDocumentIterator::setThreshold(float realThreshold)
{
    currThreshold_ = realThreshold;
    //LOG(INFO)<<"the updated threshold :"<<currThreshold_;
}

void WANDDocumentIterator::initDocIteratorSorter()
{
    if(docIteratorList_.size() < 1)
        return;

    DocumentIterator* pDocIterator;
    std::vector<DocumentIterator*>::iterator iter = docIteratorList_.begin();
    for( ; iter != docIteratorList_.end(); ++iter)
    {
        pDocIterator = (*iter);
        if(pDocIterator)
        {
            pDocIterator->setCurrent(false);
            if(pDocIterator->next())
            {
                docIteratorSorter_.insert(make_pair(pDocIterator->doc(), pDocIterator));
            }
            else
            {
                *iter = NULL;
                delete pDocIterator;
            }
        }
    }
}

bool WANDDocumentIterator::next()
{
    return do_next();
}

bool WANDDocumentIterator::findPivot() // multimap sorter as temporary variables
{
    float sumUB = 0.0F; //sum of upper bounds of all terms
    typedef std::multimap<docid_t, DocumentIterator*>::const_iterator const_map_iter;

    const_map_iter iter = docIteratorSorter_.begin();
    for(; iter != docIteratorSorter_.end(); ++iter)
    {
        DocumentIterator* pDocIterator = iter->second;
        if(pDocIterator)
        {
            //size_t index = getIndexOfPropertyId_(pDocIterator->propertyId_);
            sumUB += pDocIterator->getUB();// * propertyWeightList_[index];
            if(sumUB > currThreshold_)
            {
                pivotDoc_ = iter->first;
                return true;
            }
        }
    }
    return false;
}

bool WANDDocumentIterator::processPrePostings(docid_t target)//multimap sorter as temporary variables
{
    if(docIteratorSorter_.size() < 1)
        return false;
    docid_t nFoundId = MAX_DOC_ID;

    DocumentIterator* front = docIteratorSorter_.begin()->second;
    while (front != NULL && front->doc() < target)
    {
        nFoundId = front->skipTo(target);
        if((MAX_DOC_ID == nFoundId) || (nFoundId < target))
        {
            docIteratorSorter_.erase(docIteratorSorter_.begin());
            if (docIteratorSorter_.size() == 0)
            {
                return false;
            }
        }
        else
        {
            docIteratorSorter_.erase(docIteratorSorter_.begin());
            docIteratorSorter_.insert(make_pair(front->doc(), front));
        }
        front = docIteratorSorter_.begin()->second;
    }
    return true;
}

bool WANDDocumentIterator::do_next()
{
    do
    {
        if(docIteratorSorter_.size() == 0)
        {
            initDocIteratorSorter();
        }
        if(docIteratorSorter_.size() == 0)
            return false;

        //////
        DocumentIterator* front = docIteratorSorter_.begin()->second;
        while(front != NULL && front->isCurrent())
        {
            front->setCurrent(false);
            if(front->next())
            {
                docIteratorSorter_.erase(docIteratorSorter_.begin());
                docIteratorSorter_.insert(make_pair(front->doc(), front));
            }
            else
            {
                docIteratorSorter_.erase(docIteratorSorter_.begin());
                if(docIteratorSorter_.size() == 0)
                    return false;
            }
            front = docIteratorSorter_.begin()->second;
        }

        if (findPivot() == false)
        {
            return false;
        }

        //std::cout<<std::endl<<"===the pivot document:"<<pivotDoc_<<std::endl;
        //std::cout<<"===the current doc:"<<currDoc_<<std::endl;

        if (pivotDoc_ <= currDoc_)
        {
            if(processPrePostings(currDoc_ + 1) == false)
            {
                return false;
            }
        }
        else //pivotTerm_ > currdoc_
        {
            if(front->doc() == pivotDoc_)
            {
                currDoc_ = pivotDoc_;
            
                processPrePostings(currDoc_ );

                std::vector<DocumentIterator*>::iterator iter = docIteratorList_.begin();
                for ( ; iter != docIteratorList_.end(); ++iter)
                {
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
            else
            {
                if(processPrePostings(pivotDoc_) == false)
                    return false;
            }
        }
    }
    while(true);
}

#if SKIP_ENABLED
sf1r::docid_t WANDDocumentIterator::skipTo(sf1r::docid_t target)
{
    return do_skipTo(target);
}

sf1r::docid_t WANDDocumentIterator::do_skipTo(sf1r::docid_t target)
{
    if(docIteratorSorter_.size() == 0)
    {
        initDocIteratorSorter();
    }
    if(docIteratorSorter_.size() == 0)
    {
        currDoc_ = MAX_DOC_ID;
        return MAX_DOC_ID;
    }

    std::vector<DocumentIterator*>::iterator iter = docIteratorList_.begin();
    for ( ; iter != docIteratorList_.end(); ++iter)
    {
        DocumentIterator* pEntry = (*iter);
        if(pEntry)
        {
            pEntry->setCurrent(false);
        }
    }

    if ( processPrePostings(target) == false )
        return MAX_DOC_ID;
    else
    {
        if(next() == true)
        {
            return currDoc_;
        }
        else
        {
            currDoc_ = MAX_DOC_ID;
            return MAX_DOC_ID;
        }
    }
}

#endif

void WANDDocumentIterator::df_cmtf(
    DocumentFrequencyInProperties& dfmap,
    CollectionTermFrequencyInProperties& ctfmap,
    MaxTermFrequencyInProperties& maxtfmap)
{
    DocumentIterator* pEntry;
    std::vector<DocumentIterator*>::iterator iter = docIteratorList_.begin();
    for( ; iter != docIteratorList_.end(); ++iter )
    {
        pEntry = (*iter);
        if(pEntry)
            pEntry->df_cmtf(dfmap, ctfmap, maxtfmap);
    }
}

void WANDDocumentIterator::queryBoosting(
    double& score,
    double& weight)
{
    DocumentIterator* pEntry;
    std::vector<DocumentIterator*>::iterator iter = docIteratorList_.begin();
    for (; iter != docIteratorList_.end(); ++iter)
    {
        pEntry = (*iter);
        if (pEntry)
            pEntry->queryBoosting(score, weight);
    }
}

}
