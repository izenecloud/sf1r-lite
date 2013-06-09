/**
 * @file QueryNormalizer.h
 * @brief This class is used to normalize the query string (in UTF-8 encoding).
 */

#ifndef SF1R_QUERY_NORMALIZER_H
#define SF1R_QUERY_NORMALIZER_H

#include <util/singleton.h>
#include <string>

namespace sf1r
{

class QueryNormalizer
{
public:
    static QueryNormalizer* get()
    {
        return izenelib::util::Singleton<QueryNormalizer>::get();
    }

    /*
     * Normalize the query from @p fromStr to @p toStr.
     *
     * It consists of below steps:
     * 1. extract the tokens using white space as delimiter
     * 2. convert the alphabets in tokens to lower case
     * 3. sort the tokens in lexicographical order
     * 4. merge the final tokens together using one white space as delimiter
     *
     * For example, given the query "三星 Galaxy I9300",
     * it would be normalized to "galaxy i9300 三星".
     */
    void normalize(const std::string& fromStr, std::string& toStr);
};

} // namespace sf1r

#endif // SF1R_QUERY_NORMALIZER_H
