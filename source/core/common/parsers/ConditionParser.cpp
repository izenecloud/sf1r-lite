/**
 * @file core/common/parsers/ConditionParser.cpp
 * @author Ian Yang
 * @date Created <2010-06-10 19:32:52>
 */
#include "ConditionParser.h"

#include "ConditionOperatorTable.h"

#include <common/Keys.h>

#include <boost/algorithm/string/case_conv.hpp>

#include <iostream>

namespace sf1r {

using driver::Keys;

ConditionParser::ConditionParser()
: array_(),
  property_(),
  op_()
{}

const Value& ConditionParser::operator()(std::size_t index) const
{
    if (index < array_.size())
    {
        return array_[index];
    }
    return def<Value>();
}

bool ConditionParser::parse(const Value& condition)
{
    clearMessages();

    if (condition.type() != Value::kObjectType)
    {
        error() = "Condition must be an object.";
        return false;
    }

    property_ = asString(condition[Keys::property]);
    if (property_.empty())
    {
        error() = "Require property in condition.";
        return false;
    }

    const Value::ArrayType* arrayPtr = condition[Keys::value].getArrayPtr();
    if (arrayPtr)
    {
        array_ = *arrayPtr;
    }
    else
    {
        if (nullValue(condition[Keys::value]))
        {
            array_.resize(0);
        }
        else
        {
            array_.resize(1);
            array_.front() = condition[Keys::value];
        }
    }

    // Keys::operator_
    op_ = asString(condition["operator"]);
    boost::to_lower(op_);

    ConditionOperatorTable::table_type::const_iterator operatorInfo =
        ConditionOperatorTable::get().find(op_);

    if (operatorInfo == ConditionOperatorTable::get().end())
    {
        error() = "Unknown operator " + op_ + ".";
        return false;
    }

    if (array_.size() < operatorInfo->second.minArgCount)
    {
        error() = "Too less arguments for operator " + op_ + ".";
        return false;
    }
    if (array_.size() > operatorInfo->second.maxArgCount)
    {
        error() = "Too many arguments for operator " + op_ + ".";
        return false;
    }

    // use formal name

    op_ = operatorInfo->second.name;

    id_type_ = asString(condition["id_type"]);
    //std::cout<<"id_type"<<id_type<<std::endl;
    return true;
}

}

