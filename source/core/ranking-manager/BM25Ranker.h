/**
 * @file    BM25Ranker.h
 * @author  Jinglei Zhao & Dohyun Yun
 * @brief   BM25Ranker Implements the BM25 probabilistic model for ranking.
 * @version 1.1
 *
 * - Copyright   : iZENESoft
 * - Log
 *   - 2008.10.20 Remove index parameter
 *   - 2008.10.22 Replaced TextQuery parameter -> SearchKeywordActionItem
 *   - 2009-09-24 Ian Yang <doit.ian@gmail.com>
 *     - Query Adjustment. Rank document item one by one.
  *   - 2010-04-07 Jia Guo
 *     - Add the minimum IDF definition, make the parameter const.
 */

#ifndef _BM25RANKER_H_
#define _BM25RANKER_H_

#include "RankingDefaultParameters.h"
#include "PropertyRanker.h"

namespace sf1r {

/**
 * @brief BM25Ranker Implements the well-known BM25 probabilistic model for
 * ranking.
 */
class BM25Ranker : public PropertyRanker {
public:
    /**
     * @param k1 Parameters of the BM25 ranking formula, 1~2
     * @param b 0.75 recommended
     * @param k3 0~1000
     */
    explicit BM25Ranker(
        float k1 = BM25_RANKER_K1,
        float b = BM25_RANKER_B,
        float k3 = BM25_RANKER_K3
    )
    : k1_(k1), b_(b), k3_(k3), minIdf_(0.1)
    {}

    void setupStats(const RankQueryProperty& queryProperty);

    void calculateTermUBs(const RankQueryProperty& queryProperty, ID_FREQ_MAP_T& ub);

    float getScore(
        const RankQueryProperty& queryProperty,
        const RankDocumentProperty& documentProperty
    ) const;

    BM25Ranker* clone() const;
private:
    const float k1_;
    const float b_;
    const float k3_;
    const float minIdf_;
    std::vector<float> idfParts_;
    std::vector<float> termUBs_;
}; // end - class BM25Ranker

} // end - namespace sf1r

#endif /* BM25RANKER_H_ */
