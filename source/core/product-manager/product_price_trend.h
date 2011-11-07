#ifndef SF1R_PRODUCTMANAGER_PRODUCTPRICETREND_H
#define SF1R_PRODUCTMANAGER_PRODUCTPRICETREND_H

#include "pm_types.h"

#include <boost/shared_ptr.hpp>

namespace libcassandra {
class Cassandra;
}

namespace sf1r
{

class ProductPriceTrend
{
public:
    ProductPriceTrend();

    ~ProductPriceTrend();

    bool insertHistory(int64_t time, ProductPriceType price);

    bool getHistory(std::vector<int64_t, ProductPriceType>& price_history, int64_t from, int64_t to);

private:
    std::string collection_name;
    std::string vendor_name_;
    std::string product_name_;
    std::string product_uuid_;

    boost::shared_ptr<libcassandra::Cassandra> cassandraClient_;
};

}

#endif
