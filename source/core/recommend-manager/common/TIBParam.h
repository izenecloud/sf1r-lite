/**
 * @file TIBParam.h
 * @brief parameter to recommend "top item bundle" 
 * @author Jun Jiang
 * @date 2011-10-16
 */

#ifndef TIB_PARAM_H
#define TIB_PARAM_H

#include <string>
#include <vector>

namespace sf1r
{

struct TIBParam
{
    TIBParam();

    bool check(std::string& errorMsg) const;

    int limit;
    int minFreq;
    std::vector<std::string> selectRecommendProps;
};

} // namespace sf1r

#endif // TIB_PARAM_H
