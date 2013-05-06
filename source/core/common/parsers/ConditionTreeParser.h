#ifndef SF1R_DRIVER_PARSERS_CONDITION_TREE_PARSER_H
#define SF1R_DRIVER_PARSERS_CONDITION_TREE_PARSER_H
/**
 * @file common/parsers/ConditionTreeParser.h
 * @author Hongliang Zhao
 * @date Created <2010-06-10 20:07:36>
 */
#include <util/driver/Value.h>
#include <util/driver/Parser.h>

#include "ConditionParser.h"
#include "ConditionsTree.h"

#include <vector>
#include <stack>

namespace sf1r {
//using namespace izenelib::driver;

class ConditionTreeParser : public ::izenelib::driver::Parser
{
public:
    ConditionTreeParser()
    {
        boost::shared_ptr<ConditionsTree> tmp(new ConditionsTree());
        conditionsTree_ = tmp;
    }

    bool parse(const Value& conditions);

    boost::shared_ptr<ConditionsTree>& parsedConditions()
    {
        return conditionsTree_;
    }

    std::stack<boost::shared_ptr<ConditionsNode> > ConditionsNodeStack_;

private:
    boost::shared_ptr<ConditionsTree> conditionsTree_;

};

}

#endif // SF1R_DRIVER_PARSERS_CONDITION_TREE_PARSER_H
