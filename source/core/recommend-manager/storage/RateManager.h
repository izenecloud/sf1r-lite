/**
 * @file RateManager.h
 * @author Jun Jiang
 * @date 2012-02-03
 */

#ifndef RATE_MANAGER_H
#define RATE_MANAGER_H

#include "../common/RecTypes.h"

#include <string>

namespace sf1r
{

class RateManager
{
public:
    virtual ~RateManager() {}

    virtual void flush() {}

    virtual bool addRate(
        const std::string& userId,
        itemid_t itemId,
        rate_t rate
    ) = 0;

    virtual bool removeRate(
        const std::string& userId,
        itemid_t itemId
    ) = 0;

    virtual bool getItemRateMap(
        const std::string& userId,
        ItemRateMap& itemRateMap
    ) = 0;
};

} // namespace sf1r

#endif // RATE_MANAGER_H
