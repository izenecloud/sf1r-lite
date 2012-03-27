#include "RecommendParam.h"

namespace
{
const std::string DOCID("DOCID");
}

namespace sf1r
{

RecommendParam::RecommendParam()
    : type(RECOMMEND_TYPE_NUM)
    , queryClickFreq(0)
    , inputParam(&condition)
{
    // DOCID property is a must in recommendation result
    selectRecommendProps.push_back(DOCID);
    selectReasonProps.push_back(DOCID);
}

bool RecommendParam::check(std::string& errorMsg) const
{
    if (inputParam.limit <= 0)
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

        case BASED_ON_RANDOM:
        {
            break;
        }

        case BUY_AFTER_QUERY:
        {
            if (query.empty())
            {
                errorMsg = "This recommendation type requires keywords in request[resource].";
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
