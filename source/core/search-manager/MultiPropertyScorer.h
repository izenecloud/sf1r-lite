#ifndef MULTIPROPERTY_SCORER_H
#define MULTIPROPERTY_SCORER_H

#include "ORDocumentIterator.h"
#include <ranking-manager/RankDocumentProperty.h>

#include <ir/index_manager/index/TermDocFreqs.h>

#include <boost/thread.hpp>

namespace sf1r{
class MultiPropertyScorer:public ORDocumentIterator
{
public:
    MultiPropertyScorer(
        const property_weight_map& propertyWeightMap,
        const std::vector<unsigned int>& propertyIds
    )
    :ORDocumentIterator()
    ,indexPropertyIdList_(propertyIds)
    {
        init_(propertyWeightMap);
    }

    virtual ~MultiPropertyScorer()
    {
        for(size_t i = 0; i < termDocReadersList_.size(); i++)
        {
            std::map<termid_t, std::vector<izenelib::ir::indexmanager::TermDocFreqs*> >& termDocReaders
                        = termDocReadersList_[i];
            for(std::map<termid_t, std::vector<izenelib::ir::indexmanager::TermDocFreqs*> >::iterator it = termDocReaders.begin();
                        it != termDocReaders.end(); ++it) {
                for(size_t j =0; j<it->second.size(); j ++ ) {
                    delete it->second[j];
                }
                it->second.clear();
            }
        }
    }

public:
    void add(propertyid_t propertyId,
                  DocumentIterator* pDocIterator)
    {
        size_t index = getIndexOfProperty_(propertyId);
        boost::mutex::scoped_lock lock(mutex_);
        docIteratorList_[index] = pDocIterator;
    }
	
    void add(propertyid_t propertyId,
                  DocumentIterator* pDocIterator,
                  std::map<termid_t, std::vector<izenelib::ir::indexmanager::TermDocFreqs*> >& termDocReaders)
    {
        size_t index = getIndexOfProperty_(propertyId);
        boost::mutex::scoped_lock lock(mutex_);
        docIteratorList_[index] = pDocIterator;
        termDocReadersList_[index] = termDocReaders;
    }

    /**
     * @warn not thread-safe, use multiple instances in multiple threads.
     */
    double score(
        const std::vector<RankQueryProperty>& rankQueryProperties,
        const std::vector<boost::shared_ptr<PropertyRanker> >& propertyRankers
    );

    void print(int level=0);

private:
    size_t getIndexOfProperty_(propertyid_t propertyId)
    {
        size_t numProperties = indexPropertyIdList_.size();
        for (size_t i = 0; i < numProperties; ++i)
        {
            if(indexPropertyIdList_[i] == propertyId)
                return i;
        }
        return 0;
    }

    void init_(const property_weight_map& propertyWeightMap)
    {
        size_t numProperties = indexPropertyIdList_.size();
        propertyWeightList_.resize(numProperties);
        docIteratorList_.resize(numProperties);
        termDocReadersList_.resize(numProperties);

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
            docIteratorList_[i] = 0;
        }

    }
    std::vector<propertyid_t> indexPropertyIdList_;
    std::vector<double> propertyWeightList_;
    std::vector<std::map<termid_t, std::vector<izenelib::ir::indexmanager::TermDocFreqs*> > > termDocReadersList_;
    boost::mutex mutex_;

    ///@brief reuse in score() for performance, so score() is not thread-safe
    RankDocumentProperty rankDocumentProperty_;
};

}

#endif
