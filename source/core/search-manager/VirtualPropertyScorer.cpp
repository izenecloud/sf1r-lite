#include "VirtualPropertyScorer.h"
#include <iostream>
namespace sf1r
{

VirtualPropertyScorer::VirtualPropertyScorer(
    const property_weight_map& propertyWeightMap,
    const std::vector<unsigned int>& propertyIds)
    :pIter_(NULL)
    ,indexPropertyIdList_(propertyIds)
    ,rankQueryProperties_(propertyIds.size())
{
    DocumentIterator::scorer_ = true;
    init_(propertyWeightMap);
}

VirtualPropertyScorer::~VirtualPropertyScorer()
{
    if(pIter_) delete pIter_;
}

void VirtualPropertyScorer::init_(
    const property_weight_map& propertyWeightMap)
{
    numProperties_ = indexPropertyIdList_.size();
    propertyWeightList_.resize(numProperties_);

    for (size_t i = 0; i < numProperties_; ++i)
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

double VirtualPropertyScorer::score(
    const RankQueryProperty& rankQueryProperty,
    const boost::shared_ptr<PropertyRanker>& propertyRanker)
{
    double score = 0.0F;
    for(unsigned i = 0; i < numProperties_; ++i)
    {
        double weight = propertyWeightList_[i];
        if(weight != 0.0F)
        {
            rankDocumentProperty_.resize_and_initdata(rankQueryProperties_[i].size());
            pIter_->doc_item(rankDocumentProperty_,i);
            score += weight * propertyRankers_[i]->getScore(
                         rankQueryProperties_[i],
                         rankDocumentProperty_
                     );
        }
    }
    return score;
}


}
