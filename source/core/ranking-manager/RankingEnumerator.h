/**
 * @file    RankingEnumerator.h
 * @author  Dohyun Yun
 * @date    2008-12-17
 * @brief   This file contents the enumerators which is used in the ranking-manager.
 * @details
 *  - Log
 *      2008-12-17 : Added TextRankingType
 */

#ifndef _RANKINGENUMERATOR_H_
#define _RANKINGENUMERATOR_H_

namespace sf1r {

namespace RankingType {

/**
 * @brief This enumerator contains which textranker will be used for ranking.
 * @details
 *  - Default : Use Default type of TextRanker. Default type is given by Configuration process.
 *  - BM25    : Use BM25Ranker.
 *  - KL      : Use KL Divergence Language Model.
 *  - PLM     : Use Proximity Language Model.
 *  - None    : Don't Use TextRanker.
 */
enum TextRankingType {
    DefaultTextRanker = 0,
    BM25,
    KL,
    PLM,
    NotUseTextRanker
}; // end - enum TextRankingType

/**
 * @brief This enumerator contains which term proximity type will be used for
 * ranking.
 *
 * - Default   : Use Default type of TermProximity. It is given by
 *                Configuration process.
 * - P_MINPROX : Use Minimum pair distance measure.
 * - P_MAXPROX : Use Maximum pair distance measure.
 * - P_AVEPROX : Use Average pair distance measure.
 * - NotUseProximityRanker : Don't Use ProximityRanker.
 */
// enum TermProximityType {
//     DefaultTermProximityType = 0,
//     P_MINPROX,
//     P_MAXPROX,
//     P_AVEPROX,
//     NotUseTermProximityType
// }; // end - enum TermProximityType

/**
 * @brief This enumerator contains which proximity ranker will be used for ranking.
 * @details
 *  - Default : Use Default type of ProximityRanker. It is given by Configuration process.
 *  - BTS     : Use Binary Tree Structured algorithm (by Jinglei).
 *  - MINDIST : Use Minimum pair distance measure.
 *  - AVEDIST : Use Average pair distance measure.
 *  - MAXDIST : Use Maximum pair distance measure.
 *  - SPAN    : Use Span based proximity distance measure.
 *  - None    : Don't Use ProximityRanker.
 */
// enum ProximityRankingType {
//     DefaultProximityRanker = 0,
//     BTS,
//     MINDIST,
//     AVEDIST,
//     MAXDIST,
//     SPAN,
//     NotUseProximityRanker,
// }; // end - enum ProximityRankingType

/**
 * @brief This enumerator contains which query independent ranker will be used for ranking.
 * @details
 *  - Default          : Use Default type of Query dependent ranker. It is given by Configuration process.
 *  - DocumentStrength : Use Document Strength for ranking.
 *  - None      : Don't Use query dependent ranker.
 */
// enum QueryIndependentRankingType {
//     DefaultQueryIndependentRanker = 0,
//     DocumentStrength,
//     NotUseQueryIndependentRanker
// }; // end - enum QueryIndependentRankingType

/**
 * @brief This enumerator contains which boost ranker will be used for ranking.
 * @details
 *  - Default        : Use Default type of boost ranker. It is given by Configuration process.
 *  - Multiplication : Use Multiplication boost rankr.
 *  - None           : Don't Use query dependent ranker.
 */
// enum BoostRankingType {
//     DefaultBoostRanker = 0,
//     Multiplication,
//     NotUseBoostRanker
// }; // end - enum BoostRankingType

} // end - namespace RankingType

} // end - namespace sf1r

#endif // _RANKINGENUMERATOR_H_
