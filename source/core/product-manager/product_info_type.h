#ifndef SF1R_PRODUCTMANAGER_PRODUCTINFOTYPE_H
#define SF1R_PRODUCTMANAGER_PRODUCTINFOTYPE_H

#include "pm_def.h"
#include "pm_types.h"
#include "product_price.h"

namespace sf1r
{

class ProductInfoType
{
public:
    ProductInfoType();

    ~ProductInfoType()
    {
        price_history_.clear();
    }

public:
    std::string collection_;

    std::string vendor_name_;

    std::string product_name_;

    std::map<int64_t, ProductPrice::ProductPriceType> price_history_;
};

}

#endif
