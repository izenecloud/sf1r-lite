#ifndef SF1V5_RANKING_MANAGER_PROPERTY_RANKER_PROTOTYPE_FACTORY_H
#define SF1V5_RANKING_MANAGER_PROPERTY_RANKER_PROTOTYPE_FACTORY_H
/**
 * @file ranking-manager/PropertyRankerPrototypeFactory.h
 * @author Ian Yang
 * @date Created <2009-09-27 15:49:29>
 * @date Updated <2009-11-26 11:26:03>
 *
 * History:
 * - 2009-11-26 Ian Yang
 *   - Return clone of the prototype
 */

#include "PropertyRanker.h"

#include <configuration-manager/RankingConfigUnit.h>

#include <boost/shared_ptr.hpp>

namespace sf1r {

/**
 * Container of all available PropertyRanker. The ranker is returned by cloning
 * the prototype. Client must check that the returned pointer is not zero.
 */
class PropertyRankerPrototypeFactory
{
public:
    PropertyRankerPrototypeFactory();

    ~PropertyRankerPrototypeFactory();

    /**
     * @brief initialize rankers through \a config
     */
    void init(const RankingConfigUnit& config);

    /**
     * @brief Creates ranker by cloning prototype.
     */
    const boost::shared_ptr<PropertyRanker>
    createRanker(TextRankingType type) const
    {
        boost::shared_ptr<PropertyRanker> ranker(rankers_[type]->clone());
        return ranker;
    }

    /**
     * @brief Creates ranker does nothing
     */
    const boost::shared_ptr<PropertyRanker> createNullRanker() const;

private:
    const PropertyRanker* rankers_[NotUseTextRanker + 1];

    inline static void deleteNullRanker_(PropertyRanker*)
    {
        // empty
    }
};

} // namespace sf1r

#endif // SF1V5_RANKING_MANAGER_PROPERTY_RANKER_PROTOTYPE_FACTORY_H
