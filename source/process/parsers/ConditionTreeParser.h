#ifndef SF1R_DRIVER_PARSERS_CONDITION_TREE_PARSER_H
#define SF1R_DRIVER_PARSERS_CONDITION_TREE_PARSER_H
/**
 * @file common/parsers/ConditionTreeParser.h
 * @author Hongliang Zhao
 * @date Created <2010-06-10 20:07:36>
 */
#include <util/driver/Value.h>
#include <util/driver/Parser.h>

#include <bundles/index/IndexBundleConfiguration.h>
#include <common/Keys.h>
#include <common/parsers/ConditionsTree.h>
#include <common/parsers/ConditionParser.h>

#include <vector>

namespace sf1r {
using namespace izenelib::driver;

class ConditionTreeParser : public ::izenelib::driver::Parser
{
public:
    ConditionTreeParser(const IndexBundleSchema& indexSchema)
    : conditionsTree_(new ConditionsNode())
    , indexSchema_(indexSchema)
    {
    }

    bool parse(const Value& conditions);

    boost::shared_ptr<ConditionsNode>& parsedConditions()
    {
        return conditionsTree_;
    }

    bool parseTree(const Value& conditions, boost::shared_ptr<ConditionsNode>& pnode);

private:
    boost::shared_ptr<ConditionsNode> conditionsTree_;
    const IndexBundleSchema& indexSchema_;
};

}

#endif // SF1R_DRIVER_PARSERS_CONDITION_TREE_PARSER_H
