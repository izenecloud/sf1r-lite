#ifndef SF1R_PARSERS_GROUPING_PARSER_H
#define SF1R_PARSERS_GROUPING_PARSER_H
/**
 * @file core/common/parsers/GroupingParser.h
 * @author Ian Yang
 * @date Created <2010-06-11 18:45:52>
 */
#include <util/driver/Parser.h>
#include <util/driver/Value.h>

#include <mining-manager/faceted-submanager/GroupParam.h>

#include <string>
#include <vector>

namespace sf1r {

/// @addtogroup parsers
/// @{

/**
 * @brief Parser field \b group.
 */
using namespace izenelib::driver;
class GroupingParser : public ::izenelib::driver::Parser
{
public:
    bool parse(const Value& grouping);

    std::vector<faceted::GroupPropParam>& mutableGroupPropertyList()
    {
        return propertyList_;
    }
    const std::vector<faceted::GroupPropParam>& groupPropertyList() const
    {
        return propertyList_;
    }

private:
    std::vector<faceted::GroupPropParam> propertyList_;
};

/// @}

} // namespace sf1r

#endif // SF1R_PARSERS_GROUPING_PARSER_H
