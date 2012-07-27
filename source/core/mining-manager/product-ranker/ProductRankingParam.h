/**
 * @file ProductRankingParam.h
 * @brief the parameters used in product ranking.
 * @author Jun Jiang <jun.jiang@izenesoft.com>
 * @date Created 2012-05-17
 */

#ifndef SF1R_PRODUCT_RANKING_PARAM_H
#define SF1R_PRODUCT_RANKING_PARAM_H

#include <common/inttypes.h>
#include <string>
#include <vector>
#include <map>

namespace sf1r
{

struct ProductRankingParam
{
    const std::string& query_;

    std::vector<docid_t>& docIds_;

    std::vector<score_t>& relevanceScores_;

    typedef std::map<category_id_t, score_t> CategoryScores;
    typedef std::vector<CategoryScores> MultiCategoryScores;
    MultiCategoryScores multiCategoryScores_;

    const std::size_t docNum_;

    ProductRankingParam(
        const std::string& query,
        std::vector<docid_t>& docIds,
        std::vector<score_t>& relevanceScores
    )
        : query_(query)
        , docIds_(docIds)
        , relevanceScores_(relevanceScores)
        , docNum_(docIds.size())
    {}

    bool isValid() const
    {
        return docNum_ == relevanceScores_.size();
    }

    score_t getCategoryScore(int scoreId, category_id_t catId) const
    {
        const CategoryScores& categoryScores = multiCategoryScores_[scoreId];
        CategoryScores::const_iterator it = categoryScores.find(catId);

        if (it != categoryScores.end())
            return it->second;

        return 0;
    }
};

} // namespace sf1r

#endif // SF1R_PRODUCT_RANKING_PARAM_H
