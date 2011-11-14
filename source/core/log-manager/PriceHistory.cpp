#include "PriceHistory.h"

#include <libcassandra/cassandra.h>

#include <boost/assign/list_of.hpp>

using namespace std;
using namespace boost::assign;
using namespace libcassandra;
using namespace org::apache::cassandra;

namespace sf1r {

const string PriceHistory::cf_name("PriceHistory");

const string PriceHistory::cf_column_type;

const string PriceHistory::cf_comparator_type("LongType");

const string PriceHistory::cf_sub_comparator_type;

const string PriceHistory::cf_comment(
    "This column family stores recent two years price history for each product.\n"
    "Schema:\n\n"
    "    column family PriceHistory = list of {\n"
    "        key \"product uuid\" : list of {\n"
    "            name \"index timestamp\" : value \"product price\"\n"
    "        }\n"
    "    }\n");

const double PriceHistory::cf_row_cache_size(0);

const double PriceHistory::cf_key_cache_size(0);

const double PriceHistory::cf_read_repair_chance(0);

const vector<ColumnDef> PriceHistory::cf_column_metadata;

const int32_t PriceHistory::cf_gc_grace_seconds(0);

const string PriceHistory::cf_default_validation_class("LongType");

const int32_t PriceHistory::cf_id(0);

const int32_t PriceHistory::cf_min_compaction_threshold(0);

const int32_t PriceHistory::cf_max_compaction_threshold(0);

const int32_t PriceHistory::cf_row_cache_save_period_in_seconds(0);

const int32_t PriceHistory::cf_key_cache_save_period_in_seconds(0);

const int8_t PriceHistory::cf_replicate_on_write(-1);

const double PriceHistory::cf_merge_shards_chance(0);

const string PriceHistory::cf_key_validation_class("LexicalUUIDType");

const string PriceHistory::cf_row_cache_provider("SerializingCacheProvider");

const string PriceHistory::cf_key_alias;

const string PriceHistory::cf_compaction_strategy;

const map<string, string> PriceHistory::cf_compaction_strategy_options;

const int32_t PriceHistory::cf_row_cache_keys_to_save(0);

const map<string, string> PriceHistory::cf_compression_options = map_list_of
    ("sstable_compression", "SnappyCompressor")
    ("chunk_length_kb", "64");

PriceHistory::PriceHistory(const string& uuid)
    : ColumnFamilyBase()
    , uuid_(uuid)
    , priceHistoryPresent_(false)
{}

PriceHistory::~PriceHistory()
{}

const string& PriceHistory::getKey() const
{
    return uuid_;
}

bool PriceHistory::updateRow() const
{
    if (!CassandraConnection::instance().isEnabled() || uuid_.empty()) return false;
    if (priceHistoryPresent_) return true;
    try
    {
        for (PriceHistoryType::const_iterator it = priceHistory_.begin();
                it != priceHistory_.end(); ++it)
        {
            CassandraConnection::instance().getCassandraClient()->insertColumn(
                    toBytes(it->second),
                    uuid_,
                    cf_name,
                    toBytes(it->first),
                    CassandraConnection::instance().createTimestamp(),
                    63072000); // Keep the price history for two years at most
        }
    }
    catch (const InvalidRequestException &ire)
    {
        cout << ire.why << endl;
        return false;
    }
    return true;
}

bool PriceHistory::getRow()
{
    if (!CassandraConnection::instance().isEnabled() || uuid_.empty()) return false;
    try
    {
        ColumnParent col_parent;
        col_parent.__set_column_family(cf_name);

        SlicePredicate pred;
        //pred.slice_range.__set_count(numeric_limits<int32_t>::max());

        vector<ColumnOrSuperColumn> raw_column_list;
        CassandraConnection::instance().getCassandraClient()->getRawSlice(
                raw_column_list,
                uuid_,
                col_parent,
                pred);

        if (raw_column_list.empty()) return true;
        if (!priceHistoryPresent_)
        {
            priceHistory_.clear();
            priceHistoryPresent_ = true;
        }
        for (vector<ColumnOrSuperColumn>::const_iterator it = raw_column_list.begin();
                it != raw_column_list.end(); ++it)
        {
            priceHistory_[fromBytes<time_t>(it->column.name)] = fromBytes<ProductPriceType>(it->column.value);
        }
    }
    catch (const InvalidRequestException &ire)
    {
        cout << ire.why << endl;
        return false;
    }
    return true;
}

void PriceHistory::insertHistory(time_t timestamp, ProductPriceType price)
{
    if (!priceHistoryPresent_)
    {
        priceHistory_.clear();
        priceHistoryPresent_ = true;
    }
    priceHistory_[timestamp] = price;
}

void PriceHistory::clearHistory()
{
    priceHistory_.clear();
    priceHistoryPresent_ = false;
}

bool PriceHistory::getRangeHistory(PriceHistoryType& history, time_t from, time_t to) const
{
    if (!CassandraConnection::instance().isEnabled() || uuid_.empty()) return false;
    try
    {
        ColumnParent col_parent;
        col_parent.__set_column_family(cf_name);

        SlicePredicate pred;
        //pred.slice_range.__set_count(numeric_limits<int32_t>::max());
        if (from != -1)
            pred.slice_range.__set_start(toBytes(from));
        if (to != -1)
            pred.slice_range.__set_finish(toBytes(to));

        vector<ColumnOrSuperColumn> raw_column_list;
        CassandraConnection::instance().getCassandraClient()->getRawSlice(
                raw_column_list,
                uuid_,
                col_parent,
                pred);

        if (!raw_column_list.empty())
        {
            for (vector<ColumnOrSuperColumn>::const_iterator it = raw_column_list.begin();
                    it != raw_column_list.end(); ++it)
            {
                history[fromBytes<time_t>(it->column.name)] = fromBytes<ProductPriceType>(it->column.value);
            }
        }
    }
    catch (const InvalidRequestException &ire)
    {
        cout << ire.why << endl;
        return false;
    }
    return true;
}

bool PriceHistory::getRangeHistoryBound(ProductPriceType& lower, ProductPriceType& upper, time_t from, time_t to) const
{
    PriceHistoryType history;
    if (!getRangeHistory(history, from, to))
        return false;
    lower = history.begin()->second;
    upper = history.rbegin()->second;
    return true;
}

void PriceHistory::resetKey(const string& newUuid)
{
    if (!newUuid.empty())
        uuid_.assign(newUuid);
    priceHistoryPresent_ = false;
}

}
