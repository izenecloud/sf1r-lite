#include "CustomRankValue.h"

namespace sf1r
{

void CustomRankValue::clear()
{
    topIds.clear();
    excludeIds.clear();
}

bool CustomRankValue::empty() const
{
    return topIds.empty() && excludeIds.empty();
}

} // namespace sf1r
