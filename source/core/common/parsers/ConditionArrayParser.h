#ifndef SF1R_DRIVER_PARSERS_CONDITION_ARRAY_PARSER_H
#define SF1R_DRIVER_PARSERS_CONDITION_ARRAY_PARSER_H
/**
 * @file common/parsers/ConditionArrayParser.h
 * @author Ian Yang
 * @date Created <2010-06-10 20:07:36>
 */
#include <util/driver/Value.h>
#include <util/driver/Parser.h>

#include "ConditionParser.h"

#include <vector>

namespace sf1r {

/// @addtogroup parsers
/// @{
using namespace izenelib::driver;
/**
 * @brief Parse \b conditions field.
 */
class ConditionArrayParser : public ::izenelib::driver::Parser
{
public:
    ConditionArrayParser();

    bool parse(const Value& conditions);

    const std::vector<ConditionParser>& parsedConditions() const
    {
        return parsers_;
    }

    const ConditionParser& parsedConditions(std::size_t index) const
    {
        return parsers_[index];
    }

    std::size_t parsedConditionCount() const
    {
        return parsers_.size();
    }

private:
    std::vector<ConditionParser> parsers_;
};

/// @}

}

#endif // SF1R_DRIVER_PARSERS_CONDITION_ARRAY_PARSER_H
