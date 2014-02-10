/**
 * @file NumericPropertyNormalizer.h
 * @brief It normalizes the property's numeric value.
 *
 * The normalized range is [0, 4], which is calculated like below:
 * norm = (log(x+1) - mean) / deviation + 2
 */

#ifndef SF1R_NUMERIC_PROPERTY_NORMALIZER_H
#define SF1R_NUMERIC_PROPERTY_NORMALIZER_H

#include <common/inttypes.h>

namespace sf1r
{
class ProductScoreConfig;

class NumericPropertyNormalizer
{
public:
    NumericPropertyNormalizer(const ProductScoreConfig& config);

    score_t normalize(score_t value) const;

private:
    const ProductScoreConfig& config_;
};

} // namespace sf1r

#endif // SF1R_NUMERIC_PROPERTY_NORMALIZER_H
