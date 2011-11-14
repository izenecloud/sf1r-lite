#include "SourceCount.h"

#include <libcassandra/cassandra.h>

#include <boost/assign/list_of.hpp>

using namespace std;
using namespace boost::assign;
using namespace libcassandra;
using namespace org::apache::cassandra;

namespace sf1r {

const ColumnFamilyBase::ColumnType SourceCount::column_type = ColumnFamilyBase::COUNTER;

const string SourceCount::cf_name("SourceCount");

const string SourceCount::cf_column_type;

const string SourceCount::cf_comparator_type("UTF8Type");

const string SourceCount::cf_sub_comparator_type;

const string SourceCount::cf_comment(
    "This column family stores sources and their product counts for each collection.\n"
    "Schema:\n\n"
    "    column family SourceCount = list of {\n"
    "        key \"collection name\" : list of {\n"
    "            name \"source name\" : value \"product count\"\n"
    "        }\n"
    "    }\n");

const double SourceCount::cf_row_cache_size(0);

const double SourceCount::cf_key_cache_size(0);

const double SourceCount::cf_read_repair_chance(0);

const vector<ColumnDef> SourceCount::cf_column_metadata;

const int32_t SourceCount::cf_gc_grace_seconds(0);

const string SourceCount::cf_default_validation_class("CounterColumnType");

const int32_t SourceCount::cf_id(0);

const int32_t SourceCount::cf_min_compaction_threshold(0);

const int32_t SourceCount::cf_max_compaction_threshold(0);

const int32_t SourceCount::cf_row_cache_save_period_in_seconds(0);

const int32_t SourceCount::cf_key_cache_save_period_in_seconds(0);

const int8_t SourceCount::cf_replicate_on_write(1);

const double SourceCount::cf_merge_shards_chance(0);

const string SourceCount::cf_key_validation_class("UTF8Type");

const string SourceCount::cf_row_cache_provider("SerializingCacheProvider");

const string SourceCount::cf_key_alias;

const string SourceCount::cf_compaction_strategy;

const map<string, string> SourceCount::cf_compaction_strategy_options;

const int32_t SourceCount::cf_row_cache_keys_to_save(0);

const map<string, string> SourceCount::cf_compression_options = map_list_of
    ("sstable_compression", "SnappyCompressor")
    ("chunk_length_kb", "64");

SourceCount::SourceCount(const string& collection)
    : ColumnFamilyBase()
    , collection_(collection)
    , sourceCountPresent_(false)
{}

SourceCount::~SourceCount()
{}

const string& SourceCount::getKey() const
{
    return collection_;
}

bool SourceCount::updateRow() const
{
    if (!CassandraConnection::instance().isEnabled() || collection_.empty()) return false;
    if (!sourceCountPresent_) return true;
    try
    {
        for (SourceCountType::const_iterator it = sourceCount_.begin();
                it != sourceCount_.end(); ++it)
        {
            CassandraConnection::instance().getCassandraClient()->incCounter(
                    it->second,
                    collection_,
                    cf_name,
                    it->first);
        }
    }
    catch (const InvalidRequestException& ire)
    {
        cerr << ire.why << endl;
        return false;
    }
    return true;
}

void SourceCount::insertCounter(const string& name, int64_t value)
{
    clear();
    sourceCount_[name] = value;
}

void SourceCount::resetKey(const string& newCollection)
{
    if (newCollection.empty())
    {
        collection_.clear();
        sourceCountPresent_ = false;
    }
    else
        collection_.assign(newCollection);
}

void SourceCount::clear()
{
    if (!sourceCountPresent_)
    {
        sourceCount_.clear();
        sourceCountPresent_ = true;
    }
}

}
