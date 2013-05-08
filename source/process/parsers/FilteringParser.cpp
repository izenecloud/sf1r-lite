/**
 * @file core/common/parsers/FilteringParser.cpp
 * @author Ian Yang
 * @date Created <2010-06-11 17:31:38>
 * @update Hongliang Zhao
 * @date Created <2013-04-27>
 * @brief support bool combination
 */
#include "FilteringParser.h"
#include <common/parsers/ConditionArrayParser.h>
#include <common/parsers/ConditionTreeParser.h>
#include <common/BundleSchemaHelpers.h>
#include <common/ValueConverter.h>

namespace sf1r {

bool FilteringParser::parse_tree(const Value& conditions)
{
    clearMessages();

    ConditionTreeParser conditionsParser;
    if (!conditionsParser.parse(conditions))
    {
        error() = conditionsParser.errorMessage();
        return false;
    }
    std::stack<boost::shared_ptr<ConditionsNode> > nodeStack;
    nodeStack.push(conditionsParser.parsedConditions()->getRoot());

    post_parse_tree(filteringTreeRules_, nodeStack);
    //printTree();
    return true;
}

bool FilteringParser::post_parse_tree(std::vector<QueryFiltering::FilteringTreeValue>& filteringTreeRules
        , stack<boost::shared_ptr<ConditionsNode> >& nodeStack)
{
    if (nodeStack.empty())
    {
        return false;
    }

    boost::shared_ptr<ConditionsNode> node = nodeStack.top();

    QueryFiltering::FilteringTreeValue filteringTreeValueRelation;
    filteringTreeValueRelation.isRelationNode_ = true;
    filteringTreeValueRelation.relation_ = node->getRelation();
    filteringTreeValueRelation.childNum_ = node->getChildNum();

    filteringTreeRules.push_back(filteringTreeValueRelation);
    //deal with leaf
    for (unsigned int i = 0; i < node->getConditionLeafList().size(); ++i)
    {
        const ConditionParser& condition = node->getConditionLeafList()[i];
        QueryFiltering::FilteringTreeValue filteringTreeValue;
        QueryFiltering::FilteringType& filteringRule = filteringTreeValue.fitleringType_;
        sf1r::PropertyDataType dataType = UNKNOWN_DATA_PROPERTY_TYPE;

        if (isPropertyFilterable(indexSchema_, condition.property()))
        {
            dataType =getPropertyDataType(indexSchema_, condition.property());
            if (dataType == sf1r::UNKNOWN_DATA_PROPERTY_TYPE)
            {
                error() = "Property's data type is unknown: " +
                                condition.property();
                return false;
            }
        }

        filteringRule.operation_ = toFilteringOperation(condition.op());
        filteringRule.property_ = condition.property();

        filteringRule.values_.resize(condition.size());
        for (std::size_t v = 0; v < condition.size(); ++v)
        {
            ValueConverter::driverValue2PropertyValue(
                    dataType,
                    condition(v),
                    filteringRule.values_[v]
            );
        }
        filteringTreeRules.push_back(filteringTreeValue);
    }
    //deal with relationNode ...
    for (unsigned int i = 0; i < node->getConditionNodeList().size(); ++i)
    {
        nodeStack.push(node->getConditionNodeList()[i]);
        post_parse_tree(filteringTreeRules, nodeStack);
    }
    nodeStack.pop();
    return true;
}

bool FilteringParser::parse(const Value& conditions)
{
    clearMessages();

    if (conditions.type() == Value::kNullType)
    {
        return true;
    }
    else if (conditions.type() == Value::kObjectType)
    {
        parse_tree(conditions);
    }
    else if (conditions.type() == Value::kArrayType)
    {
        ConditionArrayParser conditionsParser;
        if (!conditionsParser.parse(conditions))
        {
            error() = conditionsParser.errorMessage();
            return false;
        }

        std::size_t conditionCount = conditionsParser.parsedConditionCount();

        std::vector<QueryFiltering::FilteringType> filteringRulesList;
        filteringRulesList.resize(conditionCount);
        
        for (std::size_t i = 0; i < conditionCount; ++i)
        {
            // create two alias
            const ConditionParser& condition =
                conditionsParser.parsedConditions(i);
            QueryFiltering::FilteringType& filteringRule = filteringRulesList[i];

            // validation
            sf1r::PropertyDataType dataType = UNKNOWN_DATA_PROPERTY_TYPE;

            if (isPropertyFilterable(indexSchema_, condition.property()))
            {
                dataType =getPropertyDataType(indexSchema_, condition.property());
                if (dataType == sf1r::UNKNOWN_DATA_PROPERTY_TYPE)
                {
                    error() = "Property's data type is unknown: " +
                                    condition.property();
                    return false;
                }
            }
            filteringRule.operation_ = toFilteringOperation(condition.op());
            filteringRule.property_ = condition.property();

            filteringRule.values_.resize(condition.size());
            for (std::size_t v = 0; v < condition.size(); ++v)
            {
                ValueConverter::driverValue2PropertyValue(
                        dataType,
                        condition(v),
                        filteringRule.values_[v]
                );
            }
        }
        
        // Translate old condition array to new condition value ;
        // Into array add a "and" relationship ConditonNode;
        if (conditionCount != 0)
        {
            QueryFiltering::FilteringTreeValue relationNode;
            relationNode.isRelationNode_ = true;
            relationNode.relation_ = "and";
            relationNode.childNum_ = conditionCount;
            filteringTreeRules_.push_back(relationNode);
        }
        for (unsigned int j = 0; j < conditionCount; ++j)
        {
            QueryFiltering::FilteringTreeValue relationNode1;
            relationNode1.fitleringType_ = filteringRulesList[j];
            relationNode1.isRelationNode_ = false;
            filteringTreeRules_.push_back(relationNode1);
        }
    }
    else
    {
        error() = "Filter conditions's type must be ObjectType or ArrayType";
        return false;
    }

    return true;
}

QueryFiltering::FilteringOperation FilteringParser::toFilteringOperation(
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

} // namespace sf1r
