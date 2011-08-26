#ifndef SF1R_PARSERS_RANGE_PARSER_H
#define SF1R_PARSERS_RANGE_PARSER_H
/**
 * @file core/common/parsers/RangeParser.h
 * @author Jun Jiang
 * @date Created <2011-08-26>
 */
#include <util/driver/Parser.h>
#include <util/driver/Value.h>

#include <bundles/index/IndexBundleConfiguration.h>

#include <string>

namespace sf1r {

/// @addtogroup parsers
/// @{

/**
 * @brief Parser field \b range.
 */
using namespace izenelib::driver;
class RangeParser : public ::izenelib::driver::Parser
{
public:
    explicit RangeParser(const IndexBundleSchema& indexSchema)
    : indexSchema_(indexSchema)
    {}

    bool parse(const Value& rangeValue);

    const std::string& rangeProperty() const
    {
        return rangeProperty_;
    }

private:
    const IndexBundleSchema& indexSchema_;

    std::string rangeProperty_;
};

/// @}

} // namespace sf1r

#endif // SF1R_PARSERS_RANGE_PARSER_H
