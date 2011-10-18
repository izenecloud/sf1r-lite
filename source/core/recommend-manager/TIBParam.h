/**
 * @file TIBParam.h
 * @brief parameter to recommend "top item bundle" 
 * @author Jun Jiang
 * @date 2011-10-16
 */

#ifndef TIB_PARAM_H
#define TIB_PARAM_H

#include <string>

namespace sf1r
{

struct TIBParam
{
    TIBParam();

    bool check(std::string& errorMsg) const;

    int limit;
    int minFreq;
};

} // namespace sf1r

#endif // TIB_PARAM_H
