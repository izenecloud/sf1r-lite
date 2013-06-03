/**
 * @file ProductScore.h
 * @brief in order to rank topK products (mainly for merchant diversity),
 * for each product, it maintains all kinds of scores used in rerank.
 * @author Jun Jiang
 * @date Created 2012-11-30
 */

#ifndef SF1R_PRODUCT_SCORE_H
#define SF1R_PRODUCT_SCORE_H

#include <common/inttypes.h>
#include <vector>
#include <algorithm> // swap

namespace sf1r
{

struct ProductScore
{
    docid_t docId_;
    score_t topKScore_;
    merchant_id_t singleMerchantId_; // used in merchant diversity
    std::vector<score_t> rankScores_;
    int topKNum_;

    ProductScore(docid_t docId, score_t topKScore, int topKNum)
        : docId_(docId)
        , topKScore_(topKScore)
        , singleMerchantId_(0)
        , topKNum_(topKNum)
    {}

    /**
     * @return true when @c this precedes @p other.
     */
    bool operator<(const ProductScore& other) const
    {
        return rankScores_ > other.rankScores_;
    }

    void swap(ProductScore& other)
    {
        using std::swap;

        swap(docId_, other.docId_);
        swap(topKScore_, other.topKScore_);
        swap(singleMerchantId_, other.singleMerchantId_);
        swap(rankScores_, other.rankScores_);
        swap(topKNum_, other.topKNum_);
    }
};

inline void swap(ProductScore& score1, ProductScore& score2)
{
    score1.swap(score2);
}

/** a list of @c ProductScore, each for one product */
typedef std::vector<ProductScore> ProductScoreList;

/** @c ProductScoreList iterator */
typedef ProductScoreList::iterator ProductScoreIter;

/** a pair of iterators for range [first, last) */
typedef std::pair<ProductScoreIter, ProductScoreIter> ProductScoreRange;

} // namespace sf1r

#endif // SF1R_PRODUCT_SCORE_H
