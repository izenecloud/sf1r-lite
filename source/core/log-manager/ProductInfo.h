/**
 * \file ProductInfo.h
 * \brief
 * \date Sep 9, 2011
 * \author Xin Liu
 */

#ifndef _PRODUCT_INFO_H_
#define _PRODUCT_INFO_H_

#include "CassandraColumnFamily.h"

#include <boost/date_time/posix_time/posix_time.h>

namespace sf1r {

class ProductInfo : public CassandraColumnFamily
{
public:
    typedef std::map<boost::posix_time::ptime, ProductPriceType> PriceHistoryType;

    ProductInfo() : CassandraColumnFamily() {}

    ~ProductInfo() {}

    DECLARE_COLUMN_FAMILY_COMMON_ROUTINES

    inline const std::string& getUuid() const
    {
        return uuid_;
    }

    inline void setUuid(const std::string& uuid)
    {
        uuid_ = uuid;
        uuidPresent_ = true;
    }

    inline bool hasUuid() const
    {
        return uuidPresent_;
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

    inline const boost::posix_time::ptime& getFromTime() const
    {
        return fromTime_;
    }

    inline void setFromTime(const boost::posix_time::ptime& fromTime)
    {
        fromTime_ = fromTime;
        fromTimePresent_ = true;
    }

    inline bool hasFromTime() const
    {
        return fromTimePresent_;
    }

    inline const boost::posix_time::ptime& getToTime() const
    {
        return toTime_;
    }

    inline void setToTime(const boost::posix_time::ptime& toTime)
    {
        toTime_ = toTime;
        toTimePresent_ = true;
    }

    inline bool hasToTime() const
    {
        return toTimePresent_;
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
    std::string uuid_;
    bool uuidPresent_;

    std::string source_;
    bool sourcePresent_;

    std::string title_;
    bool titlePresent_;

    std::string collection_;
    bool collectionPresent_;

    boost::posix_time::ptime fromTime_;
    bool fromTimePresent_;

    boost::posix_time::ptime toTime_;
    bool toTimePresent_;

    PriceHistoryType priceHistory_;
    bool priceHistoryPresent_;
};

}
#endif /*_PRODUCT_INFO_H_ */
