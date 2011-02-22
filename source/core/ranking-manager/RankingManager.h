/**
 * @file RankingManager.h
 * @author Jinglei Zhao
 * @author Dohyun Yun
 * @brief  The interface class of RankingManager component in SF1v5.0
 *
 * History
 * - 2008-10-17
 *   - Remove Index parameter which is
 * - 2008-10-22
 *   - Remove TextQuery and place actionItem parameter into all the interfaces.
 * - 2008-11-21
 *   - Remove all the Interfaces and create only one interface.
 *   - Insert one parameter (commonSet) to the getRankingScore interface.
 *   - Insert version log in the default constructor.
 * - 2009-01-14
 *   - Modify interface of ranking-manager
 * - 2009-09-22 Ian Yang
 *   - Clean dead code.
 *   - Add interface to rank one document item.
 * - 2009-09-24 Ian Yang
 *   - Use factory to manager rankers.
 *   - Refine performance for ranking documents one by one.
 */

#ifndef _RANKINGMANAGER_H_
#define _RANKINGMANAGER_H_

#include "RankingEnumerator.h"
#include "RankingScoreItem.h"
#include "PropertyRankerPrototypeFactory.h"
#include "MultiplePropertiesRanker.h"

#include <common/type_defs.h>

#include <vector>

namespace sf1r {

/**
 * @brief It is the interface component of the RankManager.
 *
 * It provides interfaces for other parts of SF1v5.0 to use the various
 * ranking mechanisms provided by RankManager.
 */
typedef std::map<propertyid_t, float> property_weight_map;
class RankingManager {

public:
    void init(const RankingConfigUnit& config);

    void setPropertyWeight(propertyid_t property, float weight)
    {
        propertyWeightMap_[property] = weight;
    }

    MultiplePropertiesRanker
    createRanker(TextRankingType textRankingType) const;

    boost::shared_ptr<PropertyRanker>
    createPropertyRanker(TextRankingType textRankingType) const;

    void createPropertyRankers(
        TextRankingType textRankingType,
        size_t property_size,
        std::vector<boost::shared_ptr<PropertyRanker> >& propertyRankers) const;

    /**
     * @brief Given a query, Get the relevance ranking score of the specified
     *        \a documentItem.
     * @param textRankingType text ranking type
     * @param collectionId id of searched collection
     * @param searchPropertyList searched properties
     * @param textQueryList query information
     * @param documentItem document to be ranked
     * @return ranking score of \a documentItem
     *
     * Use \c createRanker instead for batch ranking.
     */
    float getRankingScore(
        sf1r::TextRankingType textRankingType,
        const std::vector<propertyid_t>& searchPropertyList,
        const std::vector<RankQueryProperty>& query,
        const std::vector<RankDocumentProperty>& document
    )
    {
        return createRanker(textRankingType).getScore(
            searchPropertyList,
            query,
            document
        );
    }

    void getPropertyWeightMap(
        property_weight_map& propertyWeightMap
    )
    {
        propertyWeightMap = propertyWeightMap_;
    }
private:
    property_weight_map propertyWeightMap_;
    PropertyRankerPrototypeFactory rankerFactory_;
}; // end - class RankingManager


}

#endif
