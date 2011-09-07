/**
 * @file RankingManager.cpp
 * @author Jinglei Zhao
 * @author Dohyun Yun
 * @brief The interface class of RankingManager component in SF1v5.0
 *
 * History:
 * - 2009-09-22 Ian Yang
 *   - Add interface to rank one document item.
 */
#include <common/SFLogger.h>
#include "RankingManager.h"

#include <iostream>

#include <boost/shared_ptr.hpp>

namespace sf1r {

void RankingManager::init(const RankingConfigUnit& config)
{
    rankerFactory_.init(config);
}

MultiplePropertiesRanker RankingManager::createRanker(
    TextRankingType textRankingType
) const
{
    return MultiplePropertiesRanker(
        createPropertyRanker(textRankingType),
        propertyWeightMap_
    );
}

boost::shared_ptr<PropertyRanker>
RankingManager::createPropertyRanker(TextRankingType textRankingType) const
{
    boost::shared_ptr<PropertyRanker> propertyRanker =
        rankerFactory_.createRanker(textRankingType);

    if (!propertyRanker)
    {
        DLOG(ERROR)<<"Ranker has not be initialized"<<endl;
        propertyRanker =
            rankerFactory_.createNullRanker();
    }

    return propertyRanker;
}

void RankingManager::createPropertyRankers(
    TextRankingType textRankingType,
    size_t property_size,
    std::vector<boost::shared_ptr<PropertyRanker> >& propertyRankers) const
{
    propertyRankers.reserve(property_size);
    for(size_t i = 0; i < property_size; ++i)
    {
        propertyRankers.push_back(createPropertyRanker(textRankingType));
    }
}

} // namespace sf1r
