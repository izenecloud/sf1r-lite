///
/// @file MerchantNameParser.h
/// @brief parse merchant names in API faceted/get_merchant_score.
/// @author Jun Jiang <jun.jiang@izenesoft.com>
/// @date Created 2012-05-16
///

#ifndef SF1R_MERCHANT_NAME_PARSER_H
#define SF1R_MERCHANT_NAME_PARSER_H

#include <util/driver/Parser.h>
#include <util/driver/Value.h>

#include <string>
#include <vector>

namespace sf1r
{

class MerchantNameParser : public ::izenelib::driver::Parser
{
public:
    virtual bool parse(const izenelib::driver::Value& merchantArray);

    const std::vector<std::string>& merchantNames() const { return merchantNames_; }

private:
    std::vector<std::string> merchantNames_;
};

} // namespace sf1r

#endif // SF1R_MERCHANT_NAME_PARSER_H
