/**
 * @file ProductScoreList.h
 * @brief a score list for each product,
 *        such as relevance score, merchant score, etc.
 * @author Jun Jiang <jun.jiang@izenesoft.com>
 * @date Created 2012-05-17
 */

#ifndef SF1R_PRODUCT_SCORE_LIST_H
#define SF1R_PRODUCT_SCORE_LIST_H

#include <common/inttypes.h>
#include <vector>
#include <algorithm> // swap

namespace sf1r
{

struct ProductScoreList
{
    docid_t docId_;

    score_t relevanceScore_;

    std::vector<score_t> rankingScores_;

    merchant_id_t singleMerchantId_; // used in merchant diversity
    int diversityRound_; // the round number in merchant diversity

    count_t ctrCount_; // used in CTR rerank

    ProductScoreList(docid_t docId, score_t relevanceScore)
        : docId_(docId)
        , relevanceScore_(relevanceScore)
        , singleMerchantId_(0)
        , diversityRound_(0)
        , ctrCount_(0)
    {}

    void pushScore(score_t score)
    {
        rankingScores_.push_back(score);
    }

    bool operator<(const ProductScoreList& other) const
    {
        return rankingScores_ > other.rankingScores_;
    }

    void swap(ProductScoreList& other)
    {
        using std::swap;

        swap(docId_, other.docId_);
        swap(relevanceScore_, other.relevanceScore_);

        rankingScores_.swap(other.rankingScores_);

        swap(singleMerchantId_, other.singleMerchantId_);
        swap(diversityRound_, other.diversityRound_);
        swap(ctrCount_, other.ctrCount_);
    }
};

inline void swap(ProductScoreList& list1, ProductScoreList& list2)
{
    list1.swap(list2);
}

inline bool compareProductScoreListByCTR(
    const ProductScoreList& list1,
    const ProductScoreList& list2
)
{
    return list1.ctrCount_ > list2.ctrCount_;
}

inline bool compareProductScoreListByDiversityRound(
    const ProductScoreList& list1,
    const ProductScoreList& list2
)
{
    return list1.diversityRound_ < list2.diversityRound_;
}

/** in this score matrix, each row is a score list for a doc */
typedef std::vector<ProductScoreList> ProductScoreMatrix;

/** ProductScoreMatrix iterator */
typedef ProductScoreMatrix::iterator ProductScoreIter;

/** a pair of iterators for range [first, last) */
typedef std::pair<ProductScoreIter, ProductScoreIter> ProductScoreRange;

} // namespace sf1r

#endif // SF1R_PRODUCT_SCORE_LIST_H
