#ifndef _PRICE_HISTORY_H_
#define _PRICE_HISTORY_H_

#include "ColumnFamilyBase.h"

#include <product-manager/pm_types.h>

namespace sf1r {

class PriceHistory : public ColumnFamilyBase
{
public:
    typedef std::map<time_t, ProductPriceType> PriceHistoryType;

    PriceHistory(const std::string& uuid = "");

    ~PriceHistory();

    virtual const std::string& getKey() const;

    virtual bool updateRow() const;

    virtual bool getRow();

    virtual void resetKey(const std::string& newUuid = "");

    void insertHistory(time_t timestamp, ProductPriceType price);

    void clearHistory();

    bool getRangeHistory(PriceHistoryType& history, time_t from, time_t to) const;

    bool getRangeHistoryBound(ProductPriceType& lower, ProductPriceType& upper, time_t from, time_t to) const;

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

    ProductPriceType priceHistory_;
    bool priceHistoryPresent_;
};

}
#endif /*_PRICE_HISTORY_H_ */
