#include "RecommendSchema.h"

namespace sf1r
{

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

} // namespace sf1r
