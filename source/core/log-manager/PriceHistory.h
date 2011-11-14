#ifndef _PRICE_HISTORY_H_
#define _PRICE_HISTORY_H_

#include "ColumnFamilyBase.h"

#include <product-manager/pm_types.h>

namespace sf1r {

class PriceHistory : public ColumnFamilyBase
{
public:
    typedef std::map<time_t, ProductPriceType> PriceHistoryType;

    explicit PriceHistory(const std::string& uuid = "");

    ~PriceHistory();

    virtual const std::string& getKey() const;

    virtual bool updateRow() const;

    virtual void insert(const std::string& name, const std::string& value);

    virtual void resetKey(const std::string& newUuid = "");

    virtual void clear();

    void insert(time_t timestamp, ProductPriceType price);

    DEFINE_COLUMN_FAMILY_COMMON_ROUTINES( PriceHistory )

    inline const std::string& getUuid() const
    {
        return uuid_;
    }

    inline void setUuid(const std::string& uuid)
    {
        uuid_ = uuid;
    }

    inline bool hasUuid() const
    {
        return !uuid_.empty();
    }

    inline const PriceHistoryType& getPriceHistory() const
    {
        return priceHistory_;
    }

    inline void setPrice(const PriceHistoryType& priceHistory)
    {
        priceHistory_ = priceHistory;
        priceHistoryPresent_ = true;
    }

    inline bool hasPriceHistory() const
    {
        return priceHistoryPresent_;
    }

private:
    std::string uuid_;

    PriceHistoryType priceHistory_;
    bool priceHistoryPresent_;
};

}
#endif /*_PRICE_HISTORY_H_ */
