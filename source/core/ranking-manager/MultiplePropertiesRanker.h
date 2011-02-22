#ifndef SF1V5_RANKING_MANAGER_MULTIPLE_PROPERTIES_RANKER_H
#define SF1V5_RANKING_MANAGER_MULTIPLE_PROPERTIES_RANKER_H
/**
 * @file ranking-manager/MultiplePropertiesRanker.h
 * @author Ian Yang
 * @date Created <2009-09-24 17:36:08>
 * @date Updated <2010-03-24 15:23:57>
 */
#include "PropertyRanker.h"

#include <common/type_defs.h>

#include <map>

#include <boost/shared_ptr.hpp>

namespace sf1r {

/**
 * @brief ranking document with multiple properties
 */
class MultiplePropertiesRanker
{
    typedef std::map<propertyid_t, float> property_weight_map;

public:
    MultiplePropertiesRanker(
        const boost::shared_ptr<PropertyRanker>& ranker,
        const property_weight_map& propertyWeightMap
    )
    : ranker_(ranker)
    , propertyWeightMap_(&propertyWeightMap)
    {}

    float getScore(
        const std::vector<propertyid_t>& searchPropertyList,
        const std::vector<RankQueryProperty>& query,
        const std::vector<RankDocumentProperty>& document
    ) const;

    inline bool requireTermPosition() const
    {
        return ranker_ && ranker_->requireTermPosition();
    }

private:
    float getWeight_(propertyid_t propertyId) const
    {
        property_weight_map::const_iterator found
            = propertyWeightMap_->find(propertyId);
        if (found != propertyWeightMap_->end())
        {
            return found->second;
        }

        return 0.0F;
    }

    boost::shared_ptr<PropertyRanker> ranker_;
    const property_weight_map* propertyWeightMap_;
};

} // namespace sf1r

#endif // SF1V5_RANKING_MANAGER_MULTIPLE_PROPERTIES_RANKER_H
