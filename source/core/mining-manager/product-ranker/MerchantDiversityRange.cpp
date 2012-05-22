#include "MerchantDiversityRange.h"

#include <algorithm> // equal

namespace sf1r
{

RangeIdentity::RangeIdentity(int merchantCountIndex)
    : compareScoreCount_(merchantCountIndex)
    , merchantScoreIndex_(merchantCountIndex+1)
    , isPositiveScore_(false)
{
}

void RangeIdentity::setScoreIter(ProductScoreIter iter)
{
    isPositiveScore_ = checkScorePositive_(iter);

    std::vector<score_t>::const_iterator first = iter->rankingScores_.begin();
    compareScores_.assign(first, first + compareScoreCount_);
}

bool RangeIdentity::isEqual(ProductScoreIter iter) const
{
    return isPositiveScore_ == checkScorePositive_(iter) &&
           std::equal(compareScores_.begin(), compareScores_.end(), iter->rankingScores_.begin());
}

bool RangeIdentity::checkScorePositive_(ProductScoreIter iter) const
{
    return iter->rankingScores_[merchantScoreIndex_] > 0;
}

MerchantDiversityRange::MerchantDiversityRange(int merchantCountIndex)
    : foundBegin_(false)
    , identity_(merchantCountIndex)
{
}

bool MerchantDiversityRange::addScoreIter(ProductScoreIter iter)
{
    merchant_id_t merchantId = iter->singleMerchantId_;

    if (foundBegin_ && (merchantId == 0 || !identity_.isEqual(iter)))
        return false;

    if (merchantId)
    {
        if (! foundBegin_)
        {
            foundBegin_ = true;
            scoreRange_.first = scoreRange_.second = iter;
            identity_.setScoreIter(iter);
        }

        ++scoreRange_.second;
        iter->diversityRound_ = ++merchantRoundMap_[merchantId];
    }

    return true;
}

} // namespace sf1r
