#include "ProductInfo.h"

#include <libcassandra/cassandra.h>

#include <boost/assign/list_of.hpp>

using namespace std;
using namespace boost::assign;
using namespace boost::posix_time;
using namespace org::apache::cassandra;

namespace sf1r {

const string ProductInfo::cassandra_name("ProductInfo");

const string ProductInfo::cassandra_column_type("Super");

const string ProductInfo::cassandra_comparator_type;

const string ProductInfo::cassandra_sub_comparator_type;

const string ProductInfo::cassandra_comment;

const double ProductInfo::cassandra_row_cache_size(0);

const double ProductInfo::cassandra_key_cache_size(200000);

const double ProductInfo::cassandra_read_repair_chance(1);

const vector<ColumnDefFake> ProductInfo::cassandra_column_metadata;

const int32_t ProductInfo::cassandra_gc_grace_seconds(864000);

const string ProductInfo::cassandra_default_validation_class;

const int32_t ProductInfo::cassandra_id(0);

const int32_t ProductInfo::cassandra_min_compaction_threshold(4);

const int32_t ProductInfo::cassandra_max_compaction_threshold(22);

const int32_t ProductInfo::cassandra_row_cache_save_period_in_seconds(0);

const int32_t ProductInfo::cassandra_key_cache_save_period_in_seconds(0);

const map<string, string> ProductInfo::cassandra_compression_options
    = map_list_of("sstable_compression", "SnappyCompressor")("chunk_length_kb", "64");

bool ProductInfo::update() const
{
    try
    {
        for (PriceHistoryType::const_iterator it = priceHistory_.begin();
                it != priceHistory_.end(); ++it)
        {
            CassandraConnection::instance().getCassandraClient()->insertColumn(
                    lexical_cast<string>(it->second),
                    collectionPresent_ ? collection_ + "_" + docId_ :docId_,
                    cassandra_name,
                    "PriceHistory",
                    to_iso_string(it->first));
        }
    }
    catch (InvalidRequestException &ire)
    {
        cout << ire.why << endl;
        return false;
    }
    return true;
}

bool ProductInfo::clear() const
{
    try
    {
        ColumnPath col_path;
        col_path.__set_column_family(cassandra_name);
        CassandraConnection::instance().getCassandraClient()->remove(
                collectionPresent_ ? collection_ + "_" + docId_ :docId_,
                col_path);
    }
    catch (InvalidRequestException &ire)
    {
        cout << ire.why << endl;
        return false;
    }
    return true;
}

bool ProductInfo::get()
{
    try
    {
        ColumnParent col_parent;
        col_parent.__set_column_family(cassandra_name);
        col_parent.__set_super_column("PriceHistory");

        SlicePredicate pred;
        if (fromTimePresent_)
            pred.slice_range.__set_start(to_iso_string(fromTime_));
        if (toTimePresent_)
            pred.slice_range.__set_finish(to_iso_string(toTime_));

        vector<Column> column_list;
        CassandraConnection::instance().getCassandraClient()->getSlice(
                column_list,
                collectionPresent_ ? collection_ + "_" + docId_ :docId_,
                col_parent,
                pred);

        if (!column_list.empty())
        {
            priceHistoryPresent_ = true;
            for (vector<Column>::const_iterator it = column_list.begin();
                    it != column_list.end(); ++it)
            {
                priceHistory_[from_iso_string(it->name)] = lexical_cast<ProductPriceType>(it->value);
            }
        }
    }
    catch (InvalidRequestException &ire)
    {
        cout << ire.why << endl;
        return false;
    }
    return true;
}

void ProductInfo::reset(const string& newDocId)
{
    docId_.assign(newDocId);
    docIdPresent_ = !docId_.empty();
    collectionPresent_ = sourcePresent_ = titlePresent_ = fromTimePresent_ = toTimePresent_ = priceHistoryPresent_ = false;
}

}
