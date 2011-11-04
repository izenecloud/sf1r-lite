#ifndef SF1R_PRODUCTMANAGER_PRODUCTPRICETREND_H
#define SF1R_PRODUCTMANAGER_PRODUCTPRICETREND_H

#include "product_price.h"
#include "product_info_type.h"

#include <boost/shared_ptr.hpp>

namespace sf1r
{

class ProductPriceTrend
{
public:
    ProductPriceTrend();

    ~ProductPriceTrend();

    bool Load();

    bool Flush();

private:
    std::vector<ProductInfoType> product_info_list_;
};

}

#endif
