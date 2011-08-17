///
/// @file split_ustr.h
/// @brief split UString for group/attribute values
/// @author Jun Jiang <jun.jiang@izenesoft.com>
/// @date Created 2011-08-17
///

#ifndef SF1R_SPLIT_USTR_H_
#define SF1R_SPLIT_USTR_H_

#include <util/ustring/UString.h>

#include <vector>

namespace sf1r
{

/**
 * Split @p src into multiple group values.
 * @param src the source string, such as "T恤,衬衫,POLO衫,长袖上衣"
 * @param groupValues stores the group values, for example,
 *                    [0]: "T恤",
 *                    [1]: "衬衫",
 *                    [2]: "POLO衫",
 *                    [3]: "长袖上衣"
 */
void split_group_value(
    const izenelib::util::UString& src,
    std::vector<izenelib::util::UString>& groupValues
);

typedef std::pair<
            izenelib::util::UString, /// attribute name
            std::vector<izenelib::util::UString> /// attribute values
        > AttrPair;

/**
 * Split @p src into pairs of attribute name and values.
 * @param src the source string, such as "品牌:欧艾尼,年份:2011,尺码:S|M|L|XL"
 * @param attrPairs stores the pairs of attribute name and values, for example,
 *                  [0]: attribute name "品牌", values: "欧艾尼",
 *                  [1]: attribute name "年份", values: "2011",
 *                  [2]: attribute name "尺码", values: ["S", "M", "L", "XL"]
 */
void split_attr_pair(
    const izenelib::util::UString& src,
    std::vector<AttrPair>& attrPairs
);

}

#endif //SF1R_SPLIT_USTR_H_
