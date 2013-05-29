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
using namespace sf1r;

WANDDocumentIterator::WANDDocumentIterator()
{
    
}

WANDDocumentIterator::~WANDDocumentIterator()
{
    std::vector<TermDocumentIterator*>::iterator iter = docIteratorList_.begin();
    for (; iter != docIteratorList_.end(); ++iter)
    {
        if(*iter)
            delete *iter;
    }
}

void WANDDocumentIterator::add(DocumentIterator* pDocIterator)
{
    TermDocumentIterator* pTermDocIter = (TermDocumentIterator*)pDocIterator;
    boost::mutex::scoped_lock lock(mutex_);
    docIteratorList_.push_back(pTermDocIter);
}

void WANDDocumentIterator::setUB(bool useOriginalQuery, UpperBoundInProperties& ubmap)
{
    if (docIteratorList_.begin() == docIteratorList_.end())
        return;

    float sum = 0.0F, termThreshold = 0.0F;
    size_t nTerms = 0, nZeroTerms = 0;
    
    ID_FREQ_MAP_T& ubs = ubmap[(*docIteratorList_.begin())->property_]; 

    term_index_ub_iterator term_iter = ubs.begin();
    for(; term_iter != ubs.end(); ++term_iter)
    {
        if ((1e-6 > term_iter->second) && (-1e-6 < term_iter->second))
            nZeroTerms++;
        sum += term_iter->second;
        ++nTerms;
    }
    currThreshold_ = sum * nTerms / (nTerms - nZeroTerms);
    termThreshold = sum/nTerms * 0.8F;
    
    std::vector<TermDocumentIterator*>::iterator iter = docIteratorList_.begin();
    for( ; iter != docIteratorList_.end(); ++iter )
    {
        TermDocumentIterator* pEntry = (*iter);
        if (pEntry)
        {
            size_t termIndex = pEntry->termIndex_;
            float ub = ubs[termIndex];

            if(useOriginalQuery)
            {
                pEntry->set_ub(ub);
            }
            else
            {
                if (ub > termThreshold)
                {
                    pEntry->set_ub(ub);
                }
                else
                {
                    *iter = NULL;
                    delete pEntry;
                }
            }
        }
    }
}

void WANDDocumentIterator::initThreshold(float threshold)
{
    if(threshold > 0.0F)
    {
        currThreshold_ *= threshold;
    }
    else
    {
        currThreshold_ *= 0.5;
    }

    //LOG(INFO)<<"the initial currThreshold = "<<currThreshold_<<"for property:  "<<(*docIteratorList_.begin())->property_;
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

    TermDocumentIterator* pDocIterator;
    std::vector<TermDocumentIterator*>::iterator iter = docIteratorList_.begin();
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
    typedef std::multimap<docid_t, TermDocumentIterator*>::const_iterator const_map_iter;

    const_map_iter iter = docIteratorSorter_.begin();
    for(; iter != docIteratorSorter_.end(); ++iter)
    {
        TermDocumentIterator* pDocIterator = iter->second;
        if(pDocIterator)
        {
            //size_t index = getIndexOfPropertyId_(pDocIterator->propertyId_);
            sumUB += pDocIterator->ub_;// * propertyWeightList_[index];
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

    TermDocumentIterator* front = docIteratorSorter_.begin()->second;
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
        TermDocumentIterator* front = docIteratorSorter_.begin()->second;
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

                std::vector<TermDocumentIterator*>::iterator iter = docIteratorList_.begin();
                for ( ; iter != docIteratorList_.end(); ++iter)
                {
                    TermDocumentIterator* pEntry = (*iter);
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

    std::vector<TermDocumentIterator*>::iterator iter = docIteratorList_.begin();
    for ( ; iter != docIteratorList_.end(); ++iter)
    {
        TermDocumentIterator* pEntry = (*iter);
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
    TermDocumentIterator* pEntry;
    std::vector<TermDocumentIterator*>::iterator iter = docIteratorList_.begin();
    for( ; iter != docIteratorList_.end(); ++iter )
    {
        pEntry = (*iter);
        if(pEntry)
            pEntry->df_cmtf(dfmap, ctfmap, maxtfmap);
    }
}
