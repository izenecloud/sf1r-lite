#ifndef SF1V5_RANKING_MANAGER_TERM_PROXIMITY_UTIL_H
#define SF1V5_RANKING_MANAGER_TERM_PROXIMITY_UTIL_H
/**
 * @file sf1r/ranking-manager/TermProximityUtil.h
 * @author Ian Yang
 * @date Created <2009-09-24 14:45:28>
 * @date Updated <2010-03-24 15:23:00>
 */

#include "RankDocumentProperty.h"

namespace sf1r {

/**
 * @brief Distance of closest positions. If two words occur in same positions,
 * the distance is assumed to be 1
 */
unsigned closestPositionDistance(
    const RankDocumentProperty& documentProperty,
    RankDocumentProperty::size_type i,
    RankDocumentProperty::size_type j
);

} // namespace sf1r

#endif // SF1V5_RANKING_MANAGER_TERM_PROXIMITY_UTIL_H
