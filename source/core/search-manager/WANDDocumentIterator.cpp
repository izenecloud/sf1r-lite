/**
 * \file WANDDocumentIterator.cpp
 * \brief 
 * \date Feb 29, 2012
 * \author Xin Liu
 */

#include "WANDDocumentIterator.h"

using namespace sf1r;

WANDDocumentIterator::~WANDDocumentIterator()
{
    std::vector<TermDocumentIterator*>::iterator iter = docIteratorList_.begin();
    for (; iter != docIteratorList_.end(); ++iter)
    {
        if(*iter)
            delete *iter;
    }
}

void WANDDocumentIterator::add(TermDocumentIterator* pDocIterator)
{
    boost::mutex::scoped_lock lock(mutex_);
    docIteratorList_.push_back(pDocIterator);
}

size_t WANDDocumentIterator::getIndexOfPropertyId_(propertyid_t propertyId)
{
    size_t numProperties = indexPropertyIdList_.size();
    for (size_t i = 0; i < numProperties; ++i)
    {
        if(indexPropertyIdList_[i] == propertyId)
            return i;
    }
    return 0;
}

size_t WANDDocumentIterator::getIndexOfProperty_(const std::string& property)
{
    size_t numProperties = indexPropertyList_.size();
    for (size_t i = 0; i < numProperties; ++i)
    {
        if(indexPropertyList_[i] == property)
            return i;
    }
    return 0;
}

void WANDDocumentIterator::init_(const property_weight_map& propertyWeightMap)
{
    currDoc_ = 0;
    currThreshold_ = 0.0F;
    size_t numProperties = indexPropertyList_.size();
    propertyWeightList_.resize(numProperties);

    for (size_t i = 0; i < numProperties; ++i)
    {
        property_weight_map::const_iterator found
            = propertyWeightMap.find(indexPropertyIdList_[i]);
        if (found != propertyWeightMap.end())
        {
            propertyWeightList_[i] = found->second;
        }
        else
            propertyWeightList_[i] = 0.0F;
    }
}

void WANDDocumentIterator::set_ub(const UpperBoundInProperties& ubmap)
{
    ubmap_ = ubmap;
    std::vector<TermDocumentIterator*>::iterator iter = docIteratorList_.begin();
    for( ; iter != docIteratorList_.end(); ++iter )
    {
        TermDocumentIterator* pEntry = (*iter);
        if (pEntry)
        {
            std::string currentProperty = pEntry->property_;
            size_t termIndex = pEntry->termIndex_;
            float ub = ubmap_[currentProperty][termIndex];
            pEntry->set_ub(ub);
        }
    }
}

void WANDDocumentIterator::init_threshold(float threshold)
{
    property_name_term_index_iterator property_iter = ubmap_.begin();
    float minUB = (std::numeric_limits<float>::max) ();
    float maxUB = - minUB;
    std::string currProperty;
    size_t currIndex;
    for( ; property_iter != ubmap_.end(); ++property_iter )
    {
        currProperty = property_iter->first;
        currIndex = getIndexOfProperty_(currProperty);
        float sumUBs = 0.0F;
        term_index_ub_iterator term_iter = (property_iter->second).begin();
        for( ; term_iter != (property_iter->second).end(); ++term_iter )
        {
            sumUBs += term_iter->second;
        }
        sumUBs *= propertyWeightList_[currIndex];
        if(sumUBs > maxUB)
        {
            maxUB = sumUBs;
        }
    }
    if(threshold > 0.0F)
    {
        currThreshold_ = maxUB * threshold;
    }
    else
    {
        currThreshold_ = maxUB * 0.5;
    }

    //LOG(INFO)<<"the initial currThreshold = "<<currThreshold_;
}

void WANDDocumentIterator::set_threshold(float realThreshold)
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
            size_t index = getIndexOfPropertyId_(pDocIterator->propertyId_);
            sumUB += pDocIterator->ub_ * propertyWeightList_[index];
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
            return false;

        //std::cout<<std::endl<<"===the pivot document:"<<pivotDoc_<<std::endl;
        //std::cout<<"===the current doc:"<<currDoc_<<std::endl;

        if (pivotDoc_ <= currDoc_)
        {
            if(processPrePostings(currDoc_ + 1) == false)
                return false;
        }
        else //pivotTerm_ > currdoc_
        {
            if(front->doc() == pivotDoc_)
            {
                currDoc_ = pivotDoc_;

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
    }while(true);
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
        return false;

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
            return currDoc_;
        else
            return MAX_DOC_ID;
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

double WANDDocumentIterator::score(
    const std::vector<RankQueryProperty>& rankQueryProperties,
    const std::vector<boost::shared_ptr<PropertyRanker> >& propertyRankers
)
{
    CREATE_PROFILER ( compute_score, "SearchManager", "doSearch_: compute score in WANDDocumentIterator");
    CREATE_PROFILER ( get_doc_item, "SearchManager", "doSearch_: get doc_item");

    TermDocumentIterator* pEntry = 0;
    double score = 0.0F;

    std::vector<TermDocumentIterator*>::iterator iter = docIteratorList_.begin();
    for (; iter != docIteratorList_.end(); ++iter)
    {
        //LOG(INFO) << "Scoring the "<<indexPropertyList_[i]<<" property";
        pEntry = (*iter);
        size_t index = getIndexOfPropertyId_(pEntry->propertyId_);
        if (pEntry && pEntry->isCurrent())
        {
            //LOG(INFO) << "Current scoring the "<<indexPropertyList_[i]<<"property";
            double weight = propertyWeightList_[index];
            if (weight != 0.0F)
            {
                rankDocumentProperty_.reset();
                rankDocumentProperty_.resize(rankQueryProperties[index].size());
                pEntry->doc_item(rankDocumentProperty_);

                //START_PROFILER ( compute_score )
                score += weight * propertyRankers[index]->getScore(
                    rankQueryProperties[index],
                    rankDocumentProperty_
                );
                //STOP_PROFILER ( compute_score )
               // LOG(INFO) << "current property's sum score:"<<score;
            }
        }
    }

    if (! (score < (numeric_limits<float>::max) ()))
    {
        score = (numeric_limits<float>::max) ();
    }

    return score;
}
