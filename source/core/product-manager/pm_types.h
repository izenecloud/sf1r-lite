#ifndef SF1R_PRODUCTMANAGER_PMTYPES_H
#define SF1R_PRODUCTMANAGER_PMTYPES_H

#include <common/type_defs.h>
#include <document-manager/Document.h>
#include <boost/date_time/posix_time/posix_time.hpp>

namespace sf1r
{

typedef double ProductPriceType;
typedef Document PMDocumentType;
typedef std::vector<std::pair<boost::posix_time::ptime, ProductPriceType> > PriceHistoryType;

struct ProductInfoType
{
    std::string collection_name_;
    std::string vendor_name_;
    std::string product_name_;
    std::string product_uuid_;

    boost::posix_time::ptime from_time_, to_time_;
    PriceHistoryType price_history_;

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
};

}

#endif
