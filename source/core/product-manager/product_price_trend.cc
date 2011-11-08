#include "product_price_trend.h"

#include <log-manager/CassandraConnection.h>
#include <libcassandra/cassandra.h>

#include <boost/lexical_cast.hpp>

using namespace sf1r;

using namespace std;
using namespace libcassandra;
using namespace org::apache::cassandra;
using namespace boost::posix_time;

const string ProductPriceTrend::column_family_("ProductInfo");
const string ProductPriceTrend::super_column_("PriceHistory");

ProductPriceTrend::ProductPriceTrend()
    : cassandra_client_(CassandraConnection::instance().getCassandraClient())
{
}

ProductPriceTrend::~ProductPriceTrend()
{
}

bool ProductPriceTrend::update(const ProductInfoType& product_info)
{
    try
    {
        for (PriceHistoryType::const_iterator it = product_info.price_history_.begin();
                it != product_info.price_history_.end(); ++it)
        {
            cassandra_client_->insertColumn(
                    lexical_cast<string>(it->second),
                    product_info.product_uuid_,
                    column_family_,
                    super_column_,
                    to_iso_string(it->first)
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

bool ProductPriceTrend::clear(const ProductInfoType& product_info)
{
    try
    {
        cassandra_client_->remove(product_info.product_uuid_, column_family_, "", "");
    }
    catch (InvalidRequestException &ire)
    {
        cout << ire.why << endl;
        return false;
    }
    return true;
}

bool ProductPriceTrend::put(const ProductInfoType& product_info)
{
    return clear(product_info) && update(product_info);
}

bool ProductPriceTrend::get(ProductInfoType& product_info)
{
    try
    {
        ColumnParent col_parent;
        col_parent.__set_column_family(column_family_);
        col_parent.__set_super_column(super_column_);

        SlicePredicate pred;
        pred.slice_range.__set_start(to_iso_string(product_info.from_time_));
        pred.slice_range.__set_finish(to_iso_string(product_info.to_time_));

        vector<Column> column_list;
        cassandra_client_->getSlice(
                column_list,
                product_info.product_uuid_,
                col_parent,
                pred
                );

        PriceHistoryType& price_history = product_info.price_history_;
        for (vector<Column>::const_iterator it = column_list.begin();
                it != column_list.end(); ++it)
        {
            price_history.push_back(make_pair(from_iso_string(it->name), lexical_cast<ProductPriceType>(it->value)));
        }
    }
    catch (InvalidRequestException &ire)
    {
        cout << ire.why << endl;
        return false;
    }
    return true;
}
