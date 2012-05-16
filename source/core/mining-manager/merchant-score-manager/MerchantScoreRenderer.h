///
/// @file MerchantScoreRenderer.h
/// @brief it renders merchant score to output (such as Value, file)
/// @author Jun Jiang <jun.jiang@izenesoft.com>
/// @date Created 2012-05-16
///

#ifndef SF1R_MERCHANT_SCORE_RENDERER_H
#define SF1R_MERCHANT_SCORE_RENDERER_H

#include "MerchantScore.h"
#include <util/driver/Value.h>

#include <string>

namespace sf1r
{

class MerchantScoreRenderer
{
public:
    MerchantScoreRenderer(const MerchantStrScoreMap& scoreMap)
        : merchantStrScoreMap_(scoreMap)
    {}

    void renderToValue(izenelib::driver::Value& merchantArray) const;

    bool renderToFile(const std::string& filePath) const;

private:
    const MerchantStrScoreMap& merchantStrScoreMap_;
};

} // namespace sf1r

#endif // SF1R_MERCHANT_SCORE_RENDERER_H
