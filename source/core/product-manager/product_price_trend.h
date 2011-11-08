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

    bool update(const ProductInfoType& product_info);

    bool clear(const ProductInfoType& product_info);

    bool put(const ProductInfoType& product_info);

    bool get(ProductInfoType& product_info);

public:
    static const std::string column_family_;
    static const std::string super_column_;

private:
    boost::shared_ptr<libcassandra::Cassandra> cassandra_client_;
};

}

#endif
