#ifndef SF1R_DRIVER_PARSERS_ORDER_PARSER_H
#define SF1R_DRIVER_PARSERS_ORDER_PARSER_H
/**
 * @file core/common/parsers/OrderParser.h
 * @author Ian Yang
 * @date Created <2010-06-11 18:10:38>
 */
#include <util/driver/Value.h>
#include <util/driver/Parser.h>

#include <string>

namespace sf1r {

using namespace izenelib::driver;

class OrderParser : public ::izenelib::driver::Parser
{
public:
    bool parse(const Value& order);

    const std::string& property() const
    {
        return property_;
    }
    bool ascendant() const
    {
        return ascendant_;
    }

private:
    std::string property_;
    bool ascendant_;
};

}

#endif // SF1R_DRIVER_PARSERS_ORDER_PARSER_H
