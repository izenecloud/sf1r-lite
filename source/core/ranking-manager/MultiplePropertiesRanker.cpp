/**
 * @file core/ranking-manager/MultiplePropertiesRanker.cpp
 * @author Ian Yang
 * @date Created <2009-09-24 17:45:47>
 * @date Updated <2010-03-24 15:24:04>
 */
#include "MultiplePropertiesRanker.h"

#include <boost/assert.hpp>

#include <limits>

namespace sf1r {

float MultiplePropertiesRanker::getScore(
    const std::vector<propertyid_t>& searchPropertyList,
    const std::vector<RankQueryProperty>& query,
    const std::vector<RankDocumentProperty>& document
) const
{
    BOOST_ASSERT(ranker_);
    BOOST_ASSERT(propertyWeightMap_);
    BOOST_ASSERT(searchPropertyList.size() == query.size());
    BOOST_ASSERT(searchPropertyList.size() == document.size());

    typedef std::vector<std::string>::size_type size_type;

    float score = 0.0F;
    size_type numProperties = searchPropertyList.size();
    for (size_type i = 0; i < numProperties; ++i)
    {
        const propertyid_t property = searchPropertyList[i];
        float weight = getWeight_(property);
        if (weight != 0.0F)
        {
            score += weight * ranker_->getScore(
                query[i],
                document[i]
            );
        }
    }

    if (! (score < (numeric_limits<float>::max) ()))
    {
        score = (numeric_limits<float>::max) ();
    }

    return score;
}

} // namespace sf1r
