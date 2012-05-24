/**
 * @file MerchantDiversityRange.h
 * @brief get ProductScoreRange for merchant diversity.
 * @author Jun Jiang <jun.jiang@izenesoft.com>
 * @date Created 2012-05-22
 */

#ifndef SF1R_MERCHANT_DIVERSITY_RANGE_H
#define SF1R_MERCHANT_DIVERSITY_RANGE_H

#include "ProductScoreList.h"

#include <vector>
#include <map>

namespace sf1r
{

class RangeIdentity
{
public:
    RangeIdentity(int compareScoreCount);

    void setScoreIter(ProductScoreIter iter);

    bool isEqual(ProductScoreIter iter) const;

private:
    bool checkScorePositive_(ProductScoreIter iter) const;

private:
    const int compareScoreCount_; // the number of scores to compare
    const int merchantScoreIndex_; // the index of merchant score

    bool isPositiveScore_;

    std::vector<score_t> compareScores_;
};

class MerchantDiversityRange
{
public:
    MerchantDiversityRange(int compareScoreCount);

    /**
     * @return false if the end of range is found.
     */
    bool addScoreIter(ProductScoreIter iter);

    const ProductScoreRange& getScoreRange() const { return scoreRange_; }

private:
    std::map<merchant_id_t, int> merchantRoundMap_;

    bool foundBegin_;

    RangeIdentity identity_;

    ProductScoreRange scoreRange_;
};

} // namespace sf1r

#endif // SF1R_MERCHANT_DIVERSITY_RANGE_H
