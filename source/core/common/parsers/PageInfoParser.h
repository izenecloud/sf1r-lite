#ifndef SF1R_DRIVER_PARSERS_PAGE_INFO_PARSER_H
#define SF1R_DRIVER_PARSERS_PAGE_INFO_PARSER_H
/**
 * @file core/common/parsers/PageInfoParser.h
 * @author Ian Yang
 * @date Created <2010-06-10 19:04:14>
 */
#include <util/driver/Value.h>
#include <util/driver/Parser.h>

namespace sf1r {

using namespace izenelib::driver;
class PageInfoParser : public ::izenelib::driver::Parser
{
public:
    explicit PageInfoParser(Value::UintType offset = 0,
                            Value::UintType limit = 0)
    : offset_(offset), limit_(limit)
    {}

    Value::UintType offset() const
    {
        return offset_;
    }

    /// @brief 0 indicates no limit limit
    Value::UintType limit() const
    {
        return limit_;
    }

    void setDefault(Value::UintType offset,
                    Value::UintType limit)
    {
        offset_ = offset;
        limit_ = limit;
    }

    bool parse(const Value& value);

private:
    Value::UintType offset_;
    Value::UintType limit_;
};

}

#endif // SF1R_DRIVER_PARSERS_PAGE_INFO_PARSER_H
