#include "product_price_trend.h"

#include <log-manager/CassandraConnection.h>

using namespace sf1r;

ProductPriceTrend::ProductPriceTrend()
    : cassandraClient_(CassandraConnection::instance().getCassandraClient())
{
}

ProductPriceTrend::~ProductPriceTrend()
{
}

bool ProductPriceTrend::insertHistory(int64_t time, ProductPriceType price)
{
    return true;
}

bool ProductPriceTrend::getHistory(std::vector<int64_t, ProductPriceType>& price_history, int64_t from, int64_t to)
{
    return true;
}
