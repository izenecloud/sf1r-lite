/**
 * @file ProductFeatureParser.h
 * @brief extract product feature for forward index
 */

#ifndef PRODUCT_FEATURE_PARSER_H
#define PRODUCT_FEATURE_PARSER_H

#include <common/inttypes.h>
#include <string>
#include <vector>

namespace sf1r
{

class ProductFeatureParser
{
public:
    void getFeatureIds(
        const std::string& source,
        uint32_t& brand,
        uint32_t& model,
        std::vector<uint32_t>& featureIds);

    void getFeatureStr(
        const std::string& source,
        std::string& featureStr);

    void convertStrToIds(
        const std::string& featureStr,
        uint32_t& brand,
        uint32_t& model,
        std::vector<uint32_t>& featureIds);
};

}

#endif // PRODUCT_FEATURE_PARSER_H
