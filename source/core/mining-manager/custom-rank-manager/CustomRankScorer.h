/**
 * @file CustomRankScorer.h
 * @brief get the custom ranking score.
 * @author Jun Jiang <jun.jiang@izenesoft.com>
 * @date Created 2012-07-17
 */

#ifndef SF1R_CUSTOM_RANK_SCORER_H
#define SF1R_CUSTOM_RANK_SCORER_H

#include "CustomRankValue.h"
#include <map>

namespace sf1r
{

class CustomRankScorer
{
public:
    enum
    {
        /**
         * the base score, all positive values returned by @c getScore() would
         * be larger than this value.
         */
        CUSTOM_RANK_BASE_SCORE = 100
    };

    CustomRankScorer(const CustomRankDocId& customValue);

    /**
     * get the score of @p docId.
     * @param docId the doc id
     * @return score value, if @p docId does not belong to the custom doc id
     *         list, it would be zero; otherwise, it would be a positive value,
     *         the larger value means higher ranking of @p docId.
     */
    score_t getScore(docid_t docId) const
    {
        ScoreMap::const_iterator it = scoreMap_.find(docId);

        if (it != scoreMap_.end())
            return it->second;

        return 0;
    }

    const CustomRankDocId& getSortCustomValue() const
    {
        return sortCustomValue_;
    }

private:
    CustomRankDocId sortCustomValue_;

    typedef std::map<docid_t, score_t> ScoreMap;
    ScoreMap scoreMap_;
};

} // namespace sf1r

#endif // SF1R_CUSTOM_RANK_SCORER_H
