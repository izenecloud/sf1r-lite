/**
 * @file RateParam.h
 * @brief parameter to "rate item" 
 * @author Jun Jiang
 * @date 2011-10-17
 */

#ifndef RATE_PARAM_H
#define RATE_PARAM_H

#include "RecTypes.h"

#include <string>

namespace sf1r
{

struct RateParam
{
    RateParam();

    bool check(std::string& errorMsg) const;

    bool isAdd;
    std::string userIdStr;
    std::string itemIdStr;

    rate_t rate;
};

} // namespace sf1r

#endif // RATE_PARAM_H
