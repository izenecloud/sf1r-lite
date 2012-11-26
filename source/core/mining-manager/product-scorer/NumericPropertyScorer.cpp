#include "NumericPropertyScorer.h"
#include <common/NumericPropertyTableBase.h>
#include <algorithm> // min, max
#include <glog/logging.h>

using namespace sf1r;

NumericPropertyScorer::NumericPropertyScorer(
    const ProductScoreConfig& config,
    boost::shared_ptr<NumericPropertyTableBase> numericTable)
    : ProductScorer(config)
    , numericTable_(numericTable)
    , minValue_(0)
    , maxValue_(0)
    , minMaxDiff_(0)
{
    std::pair<float, float> minMaxValue;
    if (numericTable_->getFloatMinValue(minValue_) &&
        numericTable_->getFloatMaxValue(maxValue_))
    {
        minMaxDiff_ = maxValue_ - minValue_;
        LOG(INFO) << "minValue_: " << minValue_
                  << ", maxValue_: " << maxValue_
                  << ", minMaxDiff_: " << minMaxDiff_;
    }
    else
    {
        LOG(INFO) << "no min/max values exist in NumericPropertyTableBase";
    }
}

score_t NumericPropertyScorer::score(docid_t docId)
{
    score_t value = 0;

    if (minMaxDiff_ == 0 || !numericTable_->getFloatValue(docId, value))
        return 0;

    score_t score = (value - minValue_) / minMaxDiff_;
    score = std::max<score_t>(score, 0);
    score = std::min<score_t>(score, 1);

    return score;
}
