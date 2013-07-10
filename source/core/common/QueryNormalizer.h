/**
 * @file QueryNormalizer.h
 * @brief This class is used to normalize the query string (in UTF-8 encoding).
 */

#ifndef SF1R_QUERY_NORMALIZER_H
#define SF1R_QUERY_NORMALIZER_H

#include <util/singleton.h>
#include <string>
#include <vector>

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
    * tokenizeQuery
    * For example, given the query "三星 Galaxy I9300"
    * The result is the vector of "三星"\"Galaxy\I9300"
    */
    void tokenizeQuery(const std::string& query, std::vector<std::string>& tokens);

    /*
    * spaceQuery
    * For example, given the query "全网底价FOTILE方太CXW-189-EH16欧式吸油烟机 + JZT-FZ26GE嵌入式燃气灶"
    * The resulut is "全网底价 FOTILE 方太 CXW-189-EH16 欧式吸油烟机 JZT-FZ26GE 嵌入式燃气灶"
    */

    void spaceQuery(const std::string& query, std::string& queryOut);

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

    /*
     * Count the number of normalized characters in @p query.
     * The continuous alphabets and digits would be seemed as one character.
     *
     * For example, given the query "三星 Galaxy I9300", the normalized characters
     * would be "三", "星", "Galaxy", "I9300", so it would return 4.
     */
    std::size_t countCharNum(const std::string& query);

    /*
     * Whether the @p query is long.
     */
    bool isLongQuery(const std::string& query)
    {
        return countCharNum(query) >= LONG_QUERY_MIN_CHAR_NUM;
    }

    void getProductTypes(const std::string & query, std::vector<std::string>& productTypes,
                        std::string& newquery);

    void getMajorQueryByPunctuation(const std::string& query, std::string& majorQuery, std::string& minorQuery);

    void getMajorQueryByKeyWord(const std::string& query, std::string& majorQuery, std::string& minorQuery);

    void clearUselessWord(const std::string& query, std::string& newquery);

    void getProductTypesForApple(const std::string& query, std::string& newquery, std::vector<std::string>& productTypes);

private:
    bool isProductType(const std::string &str);

    enum
    {
        LONG_QUERY_MIN_CHAR_NUM = 8 // the mininum character number for long query
    };
};



} // namespace sf1r

#endif // SF1R_QUERY_NORMALIZER_H
