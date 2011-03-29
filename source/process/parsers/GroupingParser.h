#ifndef SF1R_PARSERS_GROUPING_PARSER_H
#define SF1R_PARSERS_GROUPING_PARSER_H
/**
 * @file core/common/parsers/GroupingParser.h
 * @author Ian Yang
 * @date Created <2010-06-11 18:45:52>
 */
#include <util/driver/Parser.h>
#include <util/driver/Value.h>

#include <bundles/index/IndexBundleConfiguration.h>

#include <string>
#include <vector>

namespace sf1r {
class GroupingOption;

/// @addtogroup parsers
/// @{

/**
 * @brief Parser field \b group.
 */
using namespace izenelib::driver;
class GroupingParser : public ::izenelib::driver::Parser
{
public:
    explicit GroupingParser(const IndexBundleSchema& indexSchema)
    : indexSchema_(indexSchema)
    {}

    bool parse(const Value& grouping);

    std::vector<GroupingOption>& mutableGroupingOptions()
    {
        return options_;
    }
    const std::vector<GroupingOption>& groupingOptions() const
    {
        return options_;
    }

private:
    bool parse(
               const Value& groupingRule,
               GroupingOption& option);

    const IndexBundleSchema& indexSchema_;

    std::vector<GroupingOption> options_;
};

/// @}

} // namespace sf1r

#endif // SF1R_PARSERS_GROUPING_PARSER_H
