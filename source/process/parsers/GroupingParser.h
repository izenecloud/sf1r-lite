#ifndef SF1R_PARSERS_GROUPING_PARSER_H
#define SF1R_PARSERS_GROUPING_PARSER_H
/**
 * @file core/common/parsers/GroupingParser.h
 * @author Ian Yang
 * @date Created <2010-06-11 18:45:52>
 */
#include <util/driver/Parser.h>
#include <util/driver/Value.h>

#include <configuration-manager/MiningSchema.h>

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
    explicit GroupingParser(const MiningSchema& miningSchema)
    : miningSchema_(miningSchema)
    {}

    bool parse(const Value& grouping);

    std::vector<std::string>& mutableGroupPropertyList()
    {
        return propertyList_;
    }
    const std::vector<std::string>& groupPropertyList() const
    {
        return propertyList_;
    }
private:
    const MiningSchema& miningSchema_;

    std::vector<std::string> propertyList_;
};

/// @}

} // namespace sf1r

#endif // SF1R_PARSERS_GROUPING_PARSER_H
