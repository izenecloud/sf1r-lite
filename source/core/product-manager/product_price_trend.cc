#include "product_price_trend.h"

#include <log-manager/CassandraConnection.h>
#include <libcassandra/cassandra.h>
#include <libcassandra/util_functions.h>

#include <boost/lexical_cast.hpp>

using namespace sf1r;

using namespace std;
using namespace libcassandra;
using namespace org::apache::cassandra;

const string ProductPriceTrend::column_family_("ProductInfo");
const string ProductPriceTrend::super_column_("PriceHistory");

ProductPriceTrend::ProductPriceTrend()
    : cassandra_client_(CassandraConnection::instance().getCassandraClient())
{
}

ProductPriceTrend::ProductPriceTrend(const ProductInfoType& product_info)
    : product_info_(product_info)
    , cassandra_client_(CassandraConnection::instance().getCassandraClient())
{
}

ProductPriceTrend::~ProductPriceTrend()
{
}

void ProductPriceTrend::setProductInfo(const ProductInfoType& product_info)
{
    product_info_ = product_info;
}

const ProductInfoType& ProductPriceTrend::getProductInfo() const
{
    return product_info_;
}

bool ProductPriceTrend::updateHistory() const
{
    try
    {
        for (PriceHistoryType::const_iterator it = product_info_.price_history_.begin();
                it != product_info_.price_history_.end(); ++it)
        {
            cassandra_client_->insertColumn(
                    lexical_cast<string>(it->second),
                    product_info_.uuid_,
                    column_family_,
                    super_column_,
                    serializeLong(it->first)
                    );
        }
    }
    catch (InvalidRequestException &ire)
    {
        cout << ire.why << endl;
        return false;
    }
    return true;
}

bool ProductPriceTrend::clearHistory() const
{
    try
    {
        cassandra_client_->removeSuperColumn(
                product_info_.uuid_,
                column_family_,
                super_column_
                );
    }
    catch (InvalidRequestException &ire)
    {
        cout << ire.why << endl;
        return false;
    }
    return true;
}

bool ProductPriceTrend::setHistory() const
{
    return clearHistory() && updateHistory();
}

bool ProductPriceTrend::getHistory()
{
    try
    {
        ColumnParent col_parent;
        col_parent.__set_column_family(column_family_);
        col_parent.__set_super_column(super_column_);

        SlicePredicate pred;
        pred.slice_range.__set_start(serializeLong(product_info_.from_time_));
        pred.slice_range.__set_finish(serializeLong(product_info_.to_time_));

        vector<Column> column_list;
        cassandra_client_->getSlice(
                column_list,
                product_info_.uuid_,
                col_parent,
                pred
                );

        for (vector<Column>::const_iterator it = column_list.begin();
                it != column_list.end(); ++it)
        {
            product_info_.setHistory(deserializeLong(it->name), lexical_cast<ProductPriceType>(it->value));
        }
    }
    catch (InvalidRequestException &ire)
    {
        cout << ire.why << endl;
        return false;
    }
    return true;
}
