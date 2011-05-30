/**
 * @file core/common/parsers/SelectArrayParser.cpp
 * @author Xin Liu
 * @date Created <2011-04-26 15:29:00>
 */
#include "SelectArrayParser.h"

namespace sf1r {

SelectArrayParser::SelectArrayParser()
{}

bool SelectArrayParser::parse(const Value& select)
{
    clearMessages();
    parsers_.clear();

    if (select.type() == Value::kNullType)
    {
        return true;
    }

    const Value::ArrayType* array = select.getPtr<Value::ArrayType>();
    if (!array)
    {
        error() = "Select field must be an array";
        return false;
    }

    if (array->size() == 0)
    {
        return true;
    }

    parsers_.push_back(SelectFieldParser());

    for (std::size_t i = 0; i < array->size(); ++i)
    {
        if (parsers_.back().parse((*array)[i]))
        {
            parsers_.push_back(SelectFieldParser());
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
