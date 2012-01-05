#include "LogServerRequestData.h"

#include <common/Utilities.h>

namespace sf1r
{

std::string UUID2DocidList::toString() const
{
    std::ostringstream oss;
    oss << Utilities::uint128ToHexString(uuid_) << " ->";

    for (size_t i = 0; i < docidList_.size(); i++)
    {
        oss << " " << docidList_[i];
    }

    return oss.str();
}

}
