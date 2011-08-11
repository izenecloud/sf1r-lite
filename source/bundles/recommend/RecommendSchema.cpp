#include "RecommendSchema.h"

namespace sf1r
{

RecommendSchema::RecommendSchema()
    : notRecResultEvent_("not_rec_result")
    , notRecInputEvent_("not_rec_input")
{
}

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
           || event == notRecResultEvent_ 
           || event == notRecInputEvent_;
}

} // namespace sf1r
