#include "product_price_trend.h"

using namespace sf1r;

ProductPriceTrend::ProductPriceTrend()
    : product_info_list_()
{
}

ProductPriceTrend::~ProductPriceTrend()
{
    product_info_list_.clear();
}

bool ProductPriceTrend::Load()
{
    return true;
}

bool ProductPriceTrend::Flush()
{
    return true;
}
