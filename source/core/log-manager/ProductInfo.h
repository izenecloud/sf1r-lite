#ifndef _PRODUCT_INFO_H_
#define _PRODUCT_INFO_H_

#include "ColumnFamilyBase.h"

#include <product-manager/pm_types.h>

namespace sf1r {

class ProductInfo : public ColumnFamilyBase
{
public:
    typedef std::map<time_t, ProductPriceType> PriceHistoryType;

    static const std::string SuperColumnName[];

    ProductInfo(const std::string& docId = "");

    ~ProductInfo();

    virtual bool updateRow() const;

    virtual bool deleteRow();

    virtual bool getRow();

    virtual void resetKey(const std::string& newDocId = "");

    void insertHistory(ProductPriceType price);

    void insertHistory(time_t timeStamp, ProductPriceType price);

    void clearHistory();

    bool getRangeHistory(PriceHistoryType& history, time_t from, time_t to) const;

    bool getRangeHistoryBound(ProductPriceType& lower, ProductPriceType& upper, time_t from, time_t to) const;

    DEFINE_COLUMN_FAMILY_COMMON_ROUTINES( ProductInfo )

    inline const std::string& getDocId() const
    {
        return docId_;
    }

    inline void setDocId(const std::string& docId)
    {
        docId_ = docId;
        docIdPresent_ = true;
    }

    inline bool hasDocId() const
    {
        return docIdPresent_;
    }

    inline const std::string& getCollection() const
    {
        return collection_;
    }

    inline void setCollection(const std::string& collection)
    {
        collection_ = collection;
        collectionPresent_ = true;
    }

    inline bool hasCollection() const
    {
        return collectionPresent_;
    }

    inline const std::string& getSource() const
    {
        return source_;
    }

    inline void setSource(const std::string& source)
    {
        source_ = source;
        sourcePresent_ = true;
    }

    inline bool hasSource() const
    {
        return sourcePresent_;
    }

    inline const std::string& getTitle() const
    {
        return title_;
    }

    inline void setTitle(const std::string& title)
    {
        title_ = title;
        titlePresent_ = true;
    }

    inline bool hasTitle() const
    {
        return titlePresent_;
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
    bool docIdPresent_;

    std::string collection_;
    bool collectionPresent_;

    std::string source_;
    bool sourcePresent_;

    std::string title_;
    bool titlePresent_;

    PriceHistoryType priceHistory_;
    bool priceHistoryPresent_;
};

}
#endif /*_PRODUCT_INFO_H_ */
