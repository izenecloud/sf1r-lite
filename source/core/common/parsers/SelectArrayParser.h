#ifndef SF1R_DRIVER_PARSERS_SELECT_ARRAY_PARSER_H
#define SF1R_DRIVER_PARSERS_SELECT_ARRAY_PARSER_H
/**
 * @file core/common/parsers/SelectArrayParser.h
 * @author Xin Liu
 * @date Created <2011-04-26 14:22:35>
 */
#include <util/driver/Value.h>
#include <util/driver/Parser.h>

#include "SelectFieldParser.h"

#include <vector>

namespace sf1r {

using namespace izenelib::driver;
/**
 * @brief Parse \b select field.
 */
class SelectArrayParser : public ::izenelib::driver::Parser
{
public:
    SelectArrayParser();

    bool parse(const Value& select);

    const std::vector<SelectFieldParser>& parsedSelect() const
    {
        return parsers_;
    }

    const SelectFieldParser& parsedSelect(std::size_t index) const
    {
        return parsers_[index];
    }

    std::size_t parsedSelectCount() const
    {
        return parsers_.size();
    }

private:
    std::vector<SelectFieldParser> parsers_;
};

}

#endif // SF1R_DRIVER_PARSERS_SELECT_ARRAY_PARSER_H
