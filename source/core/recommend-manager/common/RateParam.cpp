#include "RateParam.h"

namespace
{
const sf1r::rate_t MIN_RATE = 1;
const sf1r::rate_t MAX_RATE = 5;
}

namespace sf1r
{

RateParam::RateParam()
    : isAdd(true)
    , rate(0)
{}

bool RateParam::check(std::string& errorMsg) const
{
    if (userIdStr.empty())
    {
        errorMsg = "Require a string in request[resource][USERID].";
        return false;
    }

    if (itemIdStr.empty())
    {
        errorMsg = "Require a string in request[resource][ITEMID].";
        return false;
    }

    if (isAdd)
    {
        if (rate < MIN_RATE || rate > MAX_RATE)
        {
            errorMsg = "Require an integral value from 1 to 5 in request[resource][star].";
            return false;
        }
    }

    return true;
}

} // namespace sf1r
