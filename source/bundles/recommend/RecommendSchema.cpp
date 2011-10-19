#include "RecommendSchema.h"

namespace sf1r
{

const std::string RecommendSchema::NOT_REC_RESULT_EVENT("not_rec_result");
const std::string RecommendSchema::NOT_REC_INPUT_EVENT("not_rec_input");

const std::string RecommendSchema::BROWSE_EVENT("browse");
const std::string RecommendSchema::CART_EVENT("shopping_cart");
const std::string RecommendSchema::PURCHASE_EVENT("purchase");
const std::string RecommendSchema::RATE_EVENT("rate");

bool RecommendSchema::getUserProperty(const std::string& name, RecommendProperty& property) const
{
    for (std::vector<RecommendProperty>::const_iterator it = userSchema_.begin();
        it != userSchema_.end(); ++it)
    {
        if (it->propertyName_ == name)
        {
            property = *it;
            return true;
        }
    }

    return false;
}

bool RecommendSchema::getItemProperty(const std::string& name, RecommendProperty& property) const
{
    for (std::vector<RecommendProperty>::const_iterator it = itemSchema_.begin();
        it != itemSchema_.end(); ++it)
    {
        if (it->propertyName_ == name)
        {
            property = *it;
            return true;
        }
    }

    return false;
}

bool RecommendSchema::hasEvent(const std::string& event) const
{
    return eventSet_.find(event) != eventSet_.end()
           || event == NOT_REC_RESULT_EVENT 
           || event == NOT_REC_INPUT_EVENT;
}

} // namespace sf1r
