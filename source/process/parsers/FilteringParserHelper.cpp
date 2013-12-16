#include "FilteringParserHelper.h"

namespace sf1r {

using namespace izenelib::driver;

QueryFiltering::FilteringOperation toFilteringOperation(
    const std::string& op
)
{
    if (op == "=")
    {
        return QueryFiltering::EQUAL;
    }
    else if (op == "<>")
    {
        return QueryFiltering::NOT_EQUAL;
    }
    else if (op == "in")
    {
        return QueryFiltering::INCLUDE;
    }
    else if (op == ">")
    {
        return QueryFiltering::GREATER_THAN;
    }
    else if (op == ">=")
    {
        return QueryFiltering::GREATER_THAN_EQUAL;
    }
    else if (op == "<")
    {
        return QueryFiltering::LESS_THAN;
    }
    else if (op == "<=")
    {
        return QueryFiltering::LESS_THAN_EQUAL;
    }
    else if (op == "between")
    {
        return QueryFiltering::RANGE;
    }
    else if (op == "starts_with")
    {
        return QueryFiltering::PREFIX;
    }
    else if (op == "ends_with")
    {
        return QueryFiltering::SUFFIX;
    }
    else if (op == "contains")
    {
        return QueryFiltering::SUB_STRING;
    }
    else if (op == "not_in")
    {
        return QueryFiltering::EXCLUDE;
    }

    BOOST_ASSERT_MSG(false, "it should never go here");

    return QueryFiltering::NULL_OPERATOR;
}

bool do_paser(const ConditionParser& condition,
            const IndexBundleSchema& indexSchema,
            QueryFiltering::FilteringType& filteringRule)
{
    // validation
    sf1r::PropertyDataType dataType = UNKNOWN_DATA_PROPERTY_TYPE;

    if (isPropertyFilterable(indexSchema, condition.property()))
    {
        dataType =getPropertyDataType(indexSchema, condition.property());
        if (dataType == sf1r::UNKNOWN_DATA_PROPERTY_TYPE)
            return false;
    }
    filteringRule.operation_ = toFilteringOperation(condition.op());
    filteringRule.property_ = condition.property();

    filteringRule.values_.resize(condition.size());
    for (std::size_t v = 0; v < condition.size(); ++v)
    {
        ValueConverter::driverValue2PropertyValue(
                dataType,
                condition(v),
                filteringRule.values_[v]);
    }
    return true;
}


}
