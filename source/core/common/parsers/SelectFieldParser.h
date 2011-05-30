#ifndef SF1R_DRIVER_PARSERS_SELECT_FIELD_PARSER_H
#define SF1R_DRIVER_PARSERS_SELECT_FIELD_PARSER_H
/**
 * @file core/common/parsers/SelectFieldParser.h
 * @author Xin Liu
 * @date Created <2011-04-26 15:33:49>
 */
#include <util/driver/Value.h>
#include <util/driver/Parser.h>

#include <string>

namespace sf1r {

using namespace izenelib::driver;
class SelectFieldParser: public ::izenelib::driver::Parser
{
public:
    SelectFieldParser();

    const std::string& property() const
    {
        return property_;
    }

    const std::string& func() const
    {
        return func_;
    }

    bool parse(const Value& select);

    void swap(SelectFieldParser& other)
    {
        using std::swap;
        swap(property_, other.property_);
        swap(func_, other.func_);
        swap(error(), other.error());
    }

private:
    std::string property_;
    std::string func_;
};

inline void swap(SelectFieldParser& a, SelectFieldParser& b)
{
    a.swap(b);
}

}

#endif // SF1R_DRIVER_PARSERS_SELECT_FIELD_PARSER_H
