#ifndef SF1R_PRODUCTMANAGER_PMTYPES_H
#define SF1R_PRODUCTMANAGER_PMTYPES_H

#include <common/type_defs.h>
#include <document-manager/Document.h>

namespace sf1r
{

typedef double ProductPriceType;
typedef Document PMDocumentType;
typedef std::map<time_t, ProductPriceType> PriceHistoryType;

struct ProductInfoType
{
    std::string collection_;
    std::string source_;
    std::string name_;
    std::string uuid_;

    time_t from_time_, to_time_;
    PriceHistoryType price_history_;

    typedef PriceHistoryType::iterator iterator;
    typedef PriceHistoryType::const_iterator const_iterator;

    ProductInfoType(
            const std::string& collection = "",
            const std::string& source = "",
            const std::string& name = "",
            const std::string& uuid = "",
            time_t from_time = 0,
            time_t to_time = 0)
        : collection_(collection)
        , source_(source)
        , name_(name)
        , uuid_(uuid)
        , from_time_(from_time)
        , to_time_(to_time)
    {}

    void setHistory(time_t time_stamp, ProductPriceType price)
    {
        price_history_[time_stamp] = price;
    }

    ProductPriceType getHistory(time_t time_stamp) const
    {
        PriceHistoryType::const_iterator it = price_history_.find(time_stamp);
        if (it != price_history_.end())
            return it->second;
        else
            return 0;
    }

    std::pair<const_iterator, const_iterator> getRangeHistory(time_t from, time_t to) const
    {
        return std::make_pair(price_history_.lower_bound(from), price_history_.upper_bound(to));
    }
};

}

#endif
