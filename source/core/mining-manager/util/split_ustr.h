///
/// @file split_ustr.h
/// @brief split UString for group/attribute values
/// @author Jun Jiang <jun.jiang@izenesoft.com>
/// @date Created 2011-08-17
///

#ifndef SF1R_SPLIT_USTR_H_
#define SF1R_SPLIT_USTR_H_

#include <util/ustring/UString.h>
#include <string>
#include <vector>

namespace sf1r
{

/** Convert from @p strPath to @p ustrPath. */
void convert_to_ustr_path(
    const std::vector<std::string>& strPath,
    std::vector<izenelib::util::UString>& ustrPath);

/**
 * Split @p src into multiple group paths.
 * @param src the source string, such as "创意生活,电脑办公>网络设备,手机数码>手机通讯>手机"
 * @param groupPaths stores the group paths, for example,
 *                    [0]: ["创意生活"],
 *                    [1]: ["电脑办公", "网络设备"],
 *                    [2]: ["手机数码", "手机通讯", "手机"]
 */
void split_group_path(
    const izenelib::util::UString& src,
    std::vector<std::vector<izenelib::util::UString> >& groupPaths);

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
    std::vector<AttrPair>& attrPairs);

}

#endif //SF1R_SPLIT_USTR_H_
