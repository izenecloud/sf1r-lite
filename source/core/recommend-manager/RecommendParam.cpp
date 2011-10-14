#include "RecommendParam.h"

namespace sf1r
{

bool RecommendParam::check(std::string& errorMsg) const
{
    if (limit <= 0)
    {
        errorMsg = "Require a positive value in request[resource][max_count].";
        return false;
    }

    switch (type)
    {
        case FREQUENT_BUY_TOGETHER:
        case BUY_ALSO_BUY:
        case VIEW_ALSO_VIEW:
        {
            if (inputItems.empty())
            {
                errorMsg = "This recommendation type requires input_items in request[resource].";
                return false;
            }
            break;
        }

        case BASED_ON_EVENT:
        {
            if (userIdStr.empty())
            {
                errorMsg = "This recommendation type requires USERID in request[resource].";
                return false;
            }
            break;
        }

        case BASED_ON_BROWSE_HISTORY:
        {
            if (inputItems.empty())
            {
                if (userIdStr.empty() || sessionIdStr.empty())
                {
                    errorMsg = "This recommendation type requires either input_items or both USERID and session_id in request[resource].";
                    return false;
                }
            }
            break;
        }

        case BASED_ON_SHOP_CART:
        {
            if (inputItems.empty() && userIdStr.empty())
            {
                errorMsg = "This recommendation type requires either input_items or USERID in request[resource].";
                return false;
            }
            break;
        }

        default:
        {
            errorMsg = "Unknown recommendation type in request[resource][rec_type].";
            return false;
        }
    }

    return true;
}

} // namespace sf1r
