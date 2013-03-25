///
/// @file convert_ustr.h
/// @brief functions to convert between UString and std::string
/// @author Jun Jiang <jun.jiang@izenesoft.com>
/// @date Created 2013-03-14
///

#ifndef SF1R_CONVERT_USTR_H_
#define SF1R_CONVERT_USTR_H_

#include <util/ustring/UString.h>
#include <string>
#include <vector>

namespace sf1r
{

/** Convert from @p ustr to @p str. */
void convert_to_str(
    const izenelib::util::UString& ustr,
    std::string& str);

/** Convert from @p str to @p ustr. */
void convert_to_ustr(
    const std::string& str,
    izenelib::util::UString& ustr);

/** Convert from @p strVector to @p ustrVector. */
void convert_to_ustr_vector(
    const std::vector<std::string>& strVector,
    std::vector<izenelib::util::UString>& ustrVector);

/** Convert from @p ustrVector to @p strVector. */
void convert_to_str_vector(
    const std::vector<izenelib::util::UString>& ustrVector,
    std::vector<std::string>& strVector);

} // namespace sf1r

#endif //SF1R_CONVERT_USTR_H_
