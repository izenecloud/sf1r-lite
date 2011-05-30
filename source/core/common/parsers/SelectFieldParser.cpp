/**
 * @file core/common/parsers/SelectFieldParser.cpp
 * @author Ian Yang
 * @date Created <2010-06-10 19:32:52>
 */
#include "SelectFieldParser.h"

#include <common/Keys.h>

#include <boost/algorithm/string/case_conv.hpp>

namespace sf1r {

using driver::Keys;

SelectFieldParser::SelectFieldParser()
: property_(),
  func_()
{}

bool SelectFieldParser::parse(const Value& select)
{
    clearMessages();
    property_.clear();
    func_.clear();


    if (select.type() == Value::kObjectType)
    {
        property_ = asString(select[Keys::property]);
        boost::to_lower(property_);
        func_ = asString(select[Keys::func]);
        boost::to_lower(func_);
    }
    else
    {
        property_ = asString(select);
    }

    if (property_.empty())
    {
        error() = "Require property in select.";
        return false;
    }

    return true;
}

}

