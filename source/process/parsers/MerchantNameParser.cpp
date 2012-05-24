#include "MerchantNameParser.h"

namespace sf1r
{

bool MerchantNameParser::parse(const izenelib::driver::Value& merchantArray)
{
    if (nullValue(merchantArray))
        return true;

    if (merchantArray.type() != izenelib::driver::Value::kArrayType)
    {
        error() = "Require an array of merchant names for request[merchants].";
        return false;
    }

    for (std::size_t i = 0; i < merchantArray.size(); ++i)
    {
        std::string merchantName = asString(merchantArray(i));
        if (merchantName.empty())
        {
            error() = "Require non-empty value for each item in request[merchants].";
            return false;
        }

        merchantNames_.push_back(merchantName);
    }

    return true;
}

} // namespace sf1r
