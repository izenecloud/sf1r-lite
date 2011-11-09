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

    ProductPriceTrend(const ProductInfoType& product_info);

    ~ProductPriceTrend();

    void setProductInfo(const ProductInfoType& product_info);

    const ProductInfoType& getProductInfo() const;

    bool updateHistory() const;

    bool clearHistory() const;

    bool setHistory() const;

    bool getHistory();

public:
    static const std::string column_family_;
    static const std::string super_column_;

private:
    ProductInfoType product_info_;
    boost::shared_ptr<libcassandra::Cassandra> cassandra_client_;
};

}

#endif
