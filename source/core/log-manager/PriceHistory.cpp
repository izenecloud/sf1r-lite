#include "PriceHistory.h"

#include <libcassandra/cassandra.h>

#include <boost/assign/list_of.hpp>

using namespace std;
using namespace boost::assign;
using namespace libcassandra;
using namespace org::apache::cassandra;

namespace sf1r {

const ColumnFamilyBase::ColumnType PriceHistory::column_type = ColumnFamilyBase::NORMAL;

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
    if (!priceHistoryPresent_) return true;
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

void PriceHistory::insert(const string& name, const string& value)
{
    clear();
    priceHistory_[fromBytes<time_t>(name)] = fromBytes<ProductPriceType>(value);
}

void PriceHistory::insert(time_t timestamp, ProductPriceType price)
{
    clear();
    priceHistory_[timestamp] = price;
}

void PriceHistory::resetKey(const string& newUuid)
{
    if (newUuid.empty())
    {
        uuid_.clear();
        priceHistoryPresent_ = false;
    }
    else
        uuid_.assign(newUuid);
}

void PriceHistory::clear()
{
    if (!priceHistoryPresent_)
    {
        priceHistory_.clear();
        priceHistoryPresent_ = true;
    }
}

}
