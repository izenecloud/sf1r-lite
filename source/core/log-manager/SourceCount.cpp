#include "SourceCount.h"

#include <libcassandra/cassandra.h>

#include <boost/assign/list_of.hpp>

using namespace std;
using namespace boost::assign;
using namespace libcassandra;
using namespace org::apache::cassandra;

namespace sf1r {

bool SourceCount::is_enabled(false);

const ColumnFamilyBase::ColumnType SourceCount::column_type = ColumnFamilyBase::COUNTER;

const string SourceCount::cf_name("SourceCount");

const string SourceCount::cf_column_type;

const string SourceCount::cf_comparator_type("UTF8Type");

const string SourceCount::cf_sub_comparator_type;

const string SourceCount::cf_comment(
    "\n\nThis column family stores sources and their product counts for each collection.\n"
    "Schema:\n\n"
    "    column family SourceCount = list of {\n"
    "        key \"collection\" : list of {\n"
    "            name \"source\" : value \"count\"\n"
    "        }\n"
    "    }\n\n");

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

const string SourceCount::cf_compaction_strategy("LeveledCompactionStrategy");

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

bool SourceCount::updateMultiRow(const vector<SourceCount>& row_list)
{
    if (row_list.empty()) return true;
    if (!is_enabled) return false;
    try
    {
        map<string, map<string, vector<Mutation> > > mutation_map;
        for (vector<SourceCount>::const_iterator vit = row_list.begin();
                vit != row_list.end(); ++vit)
        {
            vector<Mutation>& mutation_list = mutation_map[vit->collection_][cf_name];
            for (SourceCountType::const_iterator mit = vit->sourceCount_.begin();
                    mit != vit->sourceCount_.end(); ++mit)
            {
                mutation_list.push_back(Mutation());
                Mutation& mut = mutation_list.back();
                mut.__isset.column_or_supercolumn = true;
                mut.column_or_supercolumn.__isset.counter_column = true;
                CounterColumn& col = mut.column_or_supercolumn.counter_column;
                col.__set_name(mit->first);
                col.__set_value(mit->second);
            }
        }

        CassandraConnection::instance().getCassandraClient()->batchMutate(mutation_map);
    }
    catch (const InvalidRequestException &ire)
    {
        cout << ire.why << endl;
        return false;
    }
    return true;
}

bool SourceCount::getMultiSlice(
        vector<SourceCount>& row_list,
        const vector<string>& key_list,
        const string& start,
        const string& finish,
        int32_t count,
        bool reversed)
{
    if (!is_enabled) return false;
    try
    {
        ColumnParent col_parent;
        col_parent.__set_column_family(cf_name);

        SlicePredicate pred;
        pred.__isset.slice_range = true;
        pred.slice_range.__set_start(start);
        pred.slice_range.__set_finish(finish);
        pred.slice_range.__set_count(count);
        pred.slice_range.__set_reversed(reversed);

        map<string, vector<ColumnOrSuperColumn> > raw_column_map;
        CassandraConnection::instance().getCassandraClient()->getMultiSlice(
                raw_column_map,
                key_list,
                col_parent,
                pred);

        row_list.reserve(row_list.size() + raw_column_map.size());
        for (map<string, vector<ColumnOrSuperColumn> >::const_iterator mit = raw_column_map.begin();
                mit != raw_column_map.end(); ++mit)
        {
            if (mit->second.empty()) continue;
            row_list.push_back(SourceCount(mit->first));
            SourceCount& source_count = row_list.back();
            for (vector<ColumnOrSuperColumn>::const_iterator vit = mit->second.begin();
                    vit != mit->second.end(); ++vit)
            {
                source_count.insertCounter(vit->counter_column.name, vit->counter_column.value);
            }
        }
    }
    catch (const InvalidRequestException &ire)
    {
        cerr << ire.why << endl;
        return false;
    }
    return true;
}

bool SourceCount::getMultiCount(
        map<string, int32_t>& count_map,
        const vector<string>& key_list,
        const string& start,
        const string& finish)
{
    if (!is_enabled) return false;
    try
    {
        ColumnParent col_parent;
        col_parent.__set_column_family(cf_name);

        SlicePredicate pred;
        pred.__isset.slice_range = true;
        pred.slice_range.__set_start(start);
        pred.slice_range.__set_finish(finish);
        //pred.slice_range.__set_count(numeric_limits<int32_t>::max());

        CassandraConnection::instance().getCassandraClient()->getMultiCount(
                count_map,
                key_list,
                col_parent,
                pred);
    }
    catch (const InvalidRequestException &ire)
    {
        cerr << ire.why << endl;
        return false;
    }
    return true;
}

bool SourceCount::updateRow() const
{
    if (!is_enabled || collection_.empty()) return false;
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
