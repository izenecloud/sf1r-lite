/**
 * @file QueryNormalizer.h
 * @brief normalize the query string for the cases such as below:
 * 1. mapping from query to label
 * 2. mapping from query to custom rank
 */

#ifndef SF1R_QUERY_NORMALIZER_H
#define SF1R_QUERY_NORMALIZER_H

#include <string>

namespace sf1r
{

class QueryNormalizer
{
public:
    /*
     * Normalize the query (in UTF-8 encoding) from @p fromStr to @p toStr.
     *
     * The normalization consists of below steps:
     * 1. extract the tokens using white space as delimiter
     * 2. convert the alphabets in tokens to lower case
     * 3. sort the tokens in lexicographical order
     * 4. merge the final tokens together using one white space as delimiter
     *
     * For example, given the query "三星 Galaxy I9300",
     * it would be normalized to "galaxy i9300 三星".
     */
    static void normalize(const std::string& fromStr, std::string& toStr);
};

} // namespace sf1r

#endif // SF1R_QUERY_NORMALIZER_H
