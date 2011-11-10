#ifndef _PRODUCT_INFO_H_
#define _PRODUCT_INFO_H_

#include "CassandraColumnFamily.h"

#include <product-manager/pm_types.h>

namespace sf1r {

class ProductInfo : public CassandraColumnFamily
{
public:
    typedef std::map<boost::posix_time::ptime, ProductPriceType> PriceHistoryType;

    ProductInfo()
        : CassandraColumnFamily()
        , docIdPresent_(false)
        , collectionPresent_(false)
        , sourcePresent_(false)
        , titlePresent_(false)
        , fromTimePresent_(false)
        , toTimePresent_(false)
        , priceHistoryPresent_(false)
    {}

    ProductInfo(const std::string& docId)
        : CassandraColumnFamily()
        , docId_(docId)
        , docIdPresent_(true)
        , collectionPresent_(false)
        , sourcePresent_(false)
        , titlePresent_(false)
        , fromTimePresent_(false)
        , toTimePresent_(false)
        , priceHistoryPresent_(false)
    {}

    ~ProductInfo() {}

    bool update() const;

    bool clear() const;

    bool get();

    void reset(const std::string& newDocId);

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
    std::string docId_;
    bool docIdPresent_;

    std::string collection_;
    bool collectionPresent_;

    std::string source_;
    bool sourcePresent_;

    std::string title_;
    bool titlePresent_;

    boost::posix_time::ptime fromTime_;
    bool fromTimePresent_;

    boost::posix_time::ptime toTime_;
    bool toTimePresent_;

    PriceHistoryType priceHistory_;
    bool priceHistoryPresent_;
};

}
#endif /*_PRODUCT_INFO_H_ */
