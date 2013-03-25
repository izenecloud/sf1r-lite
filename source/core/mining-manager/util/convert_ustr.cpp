#include "convert_ustr.h"

namespace
{
const izenelib::util::UString::EncodingType ENCODING_TYPE =
    izenelib::util::UString::UTF_8;
}

namespace sf1r
{

void convert_to_str(
    const izenelib::util::UString& ustr,
    std::string& str)
{
    ustr.convertString(str, ENCODING_TYPE);
}

void convert_to_ustr(
    const std::string& str,
    izenelib::util::UString& ustr)
{
    ustr.assign(str, ENCODING_TYPE);
}

void convert_to_ustr_vector(
    const std::vector<std::string>& strVector,
    std::vector<izenelib::util::UString>& ustrVector)
{
    const std::size_t length = strVector.size();
    ustrVector.resize(length);

    for (std::size_t i = 0; i < length; ++i)
    {
        convert_to_ustr(strVector[i], ustrVector[i]);
    }
}

void convert_to_str_vector(
    const std::vector<izenelib::util::UString>& ustrVector,
    std::vector<std::string>& strVector)
{
    const std::size_t length = ustrVector.size();
    strVector.resize(length);

    for (std::size_t i = 0; i < length; ++i)
    {
        convert_to_str(ustrVector[i], strVector[i]);
    }
}

} // namespace sf1r
