/**
 * @file core/common/parsers/PageInfoParser.cpp
 * @author Ian Yang
 * @date Created <2010-06-10 19:13:08>
 */
#include "PageInfoParser.h"
#include <common/Keys.h>

namespace sf1r {
using driver::Keys;
bool PageInfoParser::parse(const Value& value)
{
    clearMessages();

    if (value.hasKey(Keys::offset))
    {
        offset_ = asUint(value[Keys::offset]);
    }

    if (value.hasKey(Keys::limit))
    {
        limit_ = asUint(value[Keys::limit]);
    }

    return true;
}

}

