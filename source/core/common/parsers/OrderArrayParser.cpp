/**
 * @file core/driver/parsers/OrderArrayParser.cpp
 * @author Ian Yang
 * @date Created <2010-06-11 18:31:35>
 */
#include "OrderArrayParser.h"

namespace sf1r {

OrderArrayParser::OrderArrayParser()
{}

bool OrderArrayParser::parse(const Value& orders)
{
    clearMessages();
    parsers_.clear();

    if (orders.type() == Value::kNullType)
    {
        return true;
    }

    const Value::ArrayType* array = orders.getPtr<Value::ArrayType>();

    // assume that orders is just one order without wrapping in array.
    if (!array)
    {
        parsers_.push_back(OrderParser());
        if (!parsers_.back().parse(orders))
        {
            error() = parsers_.back().errorMessage();
            parsers_.clear();
            return false;
        }
        return true;
    }

    if (array->size() == 0)
    {
        return true;
    }

    parsers_.push_back(OrderParser());

    for (std::size_t i = 0; i < array->size(); ++i)
    {
        if (parsers_.back().parse((*array)[i]))
        {
            parsers_.push_back(OrderParser());
        }
        else
        {
            if (!error().empty())
            {
                error() += "\n";
            }
            error() += parsers_.back().errorMessage();
        }
    }
    parsers_.pop_back();

    return error().empty();
}

}
