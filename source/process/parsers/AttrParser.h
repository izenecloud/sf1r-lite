#ifndef SF1R_PARSERS_ATTR_PARSER_H
#define SF1R_PARSERS_ATTR_PARSER_H
/**
 * @file AttrParser.h
 * @author Jun Jiang
 * @date Created <2011-06-25>
 */
#include <util/driver/Parser.h>
#include <util/driver/Value.h>

namespace sf1r {

/// @addtogroup parsers
/// @{

/**
 * @brief Parser field \b attr.
 */
using namespace izenelib::driver;
class AttrParser : public ::izenelib::driver::Parser
{
public:
    AttrParser()
    : attrResult_(false)
    , attrTop_(0)
    {}

    bool parse(const Value& attrValue);

    const bool attrResult() const
    {
        return attrResult_;
    }

    const int attrTop() const
    {
        return attrTop_;
    }

private:
    bool attrResult_; /// true for group by attribute

    int attrTop_; /// the number of attributes to return, 0 for return all attributes
};

/// @}

} // namespace sf1r

#endif // SF1R_PARSERS_ATTR_PARSER_H
