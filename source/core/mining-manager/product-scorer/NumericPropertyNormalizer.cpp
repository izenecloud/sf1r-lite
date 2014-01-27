#include "NumericPropertyNormalizer.h"
#include <configuration-manager/ProductScoreConfig.h>
#include <util/fmath/fmath.hpp>

using namespace sf1r;

namespace
{
const score_t kNormRange = 2;
}

NumericPropertyNormalizer::NumericPropertyNormalizer(const ProductScoreConfig& config)
        : config_(config)
{
}

score_t NumericPropertyNormalizer::normalize(score_t value) const
{
    if (config_.logBase != 0)
    {
        if (value < 0)
        {
            value = 0;
        }

        value = izenelib::util::fmath::log(value+1) /
                izenelib::util::fmath::log(config_.logBase);
    }

    score_t result = (value - config_.mean) / config_.deviation;

    // limit to range [-kNormRange, kNormRange]
    if (result < -kNormRange)
    {
        result = -kNormRange;
    }
    else if (result > kNormRange)
    {
        result = kNormRange;
    }

    // move to range [0, 2*kNormRange]
    result += kNormRange;

    return result;
}
