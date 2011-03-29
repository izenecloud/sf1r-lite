#ifndef SF1R_DRIVER_PARSERS_ORDER_ARRAY_PARSER_H
#define SF1R_DRIVER_PARSERS_ORDER_ARRAY_PARSER_H
/**
 * @file core/common/parsers/OrderArrayParser.h
 * @author Ian Yang
 * @date Created <2010-06-11 18:26:37>
 */
#include <util/driver/Value.h>
#include <util/driver/Parser.h>

#include "OrderParser.h"

#include <vector>

namespace sf1r {

using namespace izenelib::driver;
class OrderArrayParser : public ::izenelib::driver::Parser
{
public:
    OrderArrayParser();

    bool parse(const Value& orders);

    const std::vector<OrderParser>& parsedOrders() const
    {
        return parsers_;
    }

    const OrderParser& parsedOrders(std::size_t index) const
    {
        return parsers_[index];
    }

    std::size_t parsedOrderCount() const
    {
        return parsers_.size();
    }

private:
    std::vector<OrderParser> parsers_;
};

}

#endif // SF1R_DRIVER_PARSERS_ORDER_ARRAY_PARSER_H
