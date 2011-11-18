#ifndef _PRICE_HISTORY_H_
#define _PRICE_HISTORY_H_

#include "ColumnFamilyBase.h"

#include <product-manager/product_price.h>

namespace sf1r {

class ProductPrice;

class PriceHistory : public ColumnFamilyBase
{
public:
    typedef std::map<time_t, ProductPrice> PriceHistoryType;

    explicit PriceHistory(const std::string& docId = "");

    ~PriceHistory();

    virtual const std::string& getKey() const;

    virtual bool updateRow() const;

    virtual bool insert(const std::string& name, const std::string& value);

    virtual void resetKey(const std::string& newDocId = "");

    virtual void clear();

    void insert(time_t timestamp, ProductPrice price);

    DEFINE_COLUMN_FAMILY_COMMON_ROUTINES( PriceHistory )

    inline const std::string& getDocId() const
    {
        return docId_;
    }

    inline void setDocId(const std::string& docId)
    {
        docId_ = docId;
    }

    inline bool hasDocId() const
    {
        return !docId_.empty();
    }

    inline const PriceHistoryType& getPriceHistory() const
    {
        return priceHistory_;
    }

    inline void setPriceHistory(const PriceHistoryType& priceHistory)
    {
        priceHistory_ = priceHistory;
        priceHistoryPresent_ = true;
    }

    inline bool hasPriceHistory() const
    {
        return priceHistoryPresent_;
    }

private:
    std::string docId_;

    PriceHistoryType priceHistory_;
    bool priceHistoryPresent_;
};

}
#endif /*_PRICE_HISTORY_H_ */
