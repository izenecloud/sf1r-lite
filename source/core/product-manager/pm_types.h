#ifndef SF1R_PRODUCTMANAGER_PMTYPES_H
#define SF1R_PRODUCTMANAGER_PMTYPES_H

#include <common/type_defs.h>
#include <document-manager/Document.h>
#include <boost/date_time/posix_time/posix_time.hpp>

namespace sf1r
{

typedef double ProductPriceType;
typedef Document PMDocumentType;
typedef std::map<boost::posix_time::ptime, ProductPriceType> PriceHistoryType;

struct ProductInfoType
{
    std::string collection_name_;
    std::string vendor_name_;
    std::string product_name_;
    std::string product_uuid_;

    boost::posix_time::ptime from_time_, to_time_;
    PriceHistoryType price_history_;

    typedef PriceHistoryType::iterator iterator;
    typedef PriceHistoryType::const_iterator const_iterator;

    ProductInfoType()
        : from_time_(), to_time_()
    {}

    ProductInfoType(
            const std::string& collection_name,
            const std::string& vendor_name,
            const std::string& product_name,
            const std::string& product_uuid,
            boost::posix_time::ptime from_time,
            boost::posix_time::ptime to_time)
        : collection_name_(collection_name)
        , vendor_name_(vendor_name)
        , product_name_(product_name)
        , product_uuid_(product_uuid)
        , from_time_(from_time), to_time_(to_time)
    {}

    void setHistory(boost::posix_time::ptime time_stamp, ProductPriceType price)
    {
        price_history_[time_stamp] = price;
    }

    ProductPriceType getHistory(boost::posix_time::ptime time_stamp) const
    {
        PriceHistoryType::const_iterator it = price_history_.find(time_stamp);
        if (it != price_history_.end())
            return it->second;
        else
            return 0;
    }

    std::pair<const_iterator, const_iterator> getRangeHistory(boost::posix_time::ptime from, boost::posix_time::ptime to) const
    {
        return std::make_pair(price_history_.lower_bound(from), price_history_.upper_bound(to));
    }
};

}

#endif
