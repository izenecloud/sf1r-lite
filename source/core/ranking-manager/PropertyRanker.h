#ifndef SF1V5_RANKING_MANAGER_PROPERTY_RANKER_H
#define SF1V5_RANKING_MANAGER_PROPERTY_RANKER_H
/**
 * @file ranking-manager/PropertyRanker.h
 * @author Ian Yang
 * @date Created <2009-09-27 14:53:40>
 * @date Updated <2010-03-24 14:48:58>
 */
#include "RankQueryProperty.h"
#include "RankDocumentProperty.h"

namespace sf1r {
class PropertyRanker
{
public:
    virtual ~PropertyRanker()
    {}

    virtual void setupStats(const RankQueryProperty& queryProperty){}

    virtual float getScore(
        const RankQueryProperty& queryProperty,
        const RankDocumentProperty& documentProperty
    ) const = 0;

    /**
     * @brief tells whether this ranker requires term position information,
     * default is no.
     */
    virtual bool requireTermPosition() const
    {
        return false;
    }

    virtual PropertyRanker* clone() const = 0;
};
} // namespace sf1r

#endif // SF1V5_RANKING_MANAGER_PROPERTY_RANKER_H
