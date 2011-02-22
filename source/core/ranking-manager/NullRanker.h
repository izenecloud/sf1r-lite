#ifndef SF1V5_RANKING_MANAGER_NULL_RANKER_H
#define SF1V5_RANKING_MANAGER_NULL_RANKER_H
/**
 * @file sf1r/ranking-manager/NullRanker.h
 * @author Ian Yang
 * @date Created <2009-09-24 17:12:49>
 * @date Updated <2010-03-24 14:52:58>
 */
#include "PropertyRanker.h"

namespace sf1r {

struct NullRanker : public PropertyRanker
{
    float getScore(const RankQueryProperty&, const RankDocumentProperty&) const
    {
        return 1.0F;
    }

    NullRanker* clone() const
    {
        return new NullRanker;
    }
};

} // namespace sf1r

#endif // SF1V5_RANKING_MANAGER_NULL_RANKER_H
