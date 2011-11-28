#include "PriceHistory.h"

#include <libcassandra/cassandra.h>
#include <libcassandra/util_functions.h>

#include <boost/assign/list_of.hpp>

using namespace std;
using namespace boost::assign;
using namespace libcassandra;
using namespace org::apache::cassandra;

namespace sf1r {

bool PriceHistory::is_enabled(false);

const ColumnFamilyBase::ColumnType PriceHistory::column_type = ColumnFamilyBase::NORMAL;

const string PriceHistory::cf_name("PriceHistory");

const string PriceHistory::cf_column_type;

const string PriceHistory::cf_comparator_type("LongType");

const string PriceHistory::cf_sub_comparator_type;

const string PriceHistory::cf_comment(
    "\n\nThis column family stores recent two years price history for each product.\n"
    "Schema:\n\n"
    "    column family PriceHistory = list of {\n"
    "        key \"docid\" : list of {\n"
    "            name \"timestamp\" : value \"price\"\n"
    "        }\n"
    "    }\n\n");

const double PriceHistory::cf_row_cache_size(0);

const double PriceHistory::cf_key_cache_size(0);

const double PriceHistory::cf_read_repair_chance(0);

const vector<ColumnDef> PriceHistory::cf_column_metadata;

const int32_t PriceHistory::cf_gc_grace_seconds(0);

const string PriceHistory::cf_default_validation_class;

const int32_t PriceHistory::cf_id(0);

const int32_t PriceHistory::cf_min_compaction_threshold(0);

const int32_t PriceHistory::cf_max_compaction_threshold(0);

const int32_t PriceHistory::cf_row_cache_save_period_in_seconds(0);

const int32_t PriceHistory::cf_key_cache_save_period_in_seconds(0);

const int8_t PriceHistory::cf_replicate_on_write(-1);

const double PriceHistory::cf_merge_shards_chance(0);

const string PriceHistory::cf_key_validation_class("UTF8Type");

const string PriceHistory::cf_row_cache_provider("SerializingCacheProvider");

const string PriceHistory::cf_key_alias;

const string PriceHistory::cf_compaction_strategy("LeveledCompactionStrategy");

const map<string, string> PriceHistory::cf_compaction_strategy_options;

const int32_t PriceHistory::cf_row_cache_keys_to_save(0);

const map<string, string> PriceHistory::cf_compression_options = map_list_of
    ("sstable_compression", "SnappyCompressor")
    ("chunk_length_kb", "64");

PriceHistory::PriceHistory(const string& docId)
    : ColumnFamilyBase()
    , docId_(docId)
    , priceHistoryPresent_(false)
{}

PriceHistory::~PriceHistory()
{}

const string& PriceHistory::getKey() const
{
    return docId_;
}

bool PriceHistory::updateMultiRow(const vector<PriceHistory>& row_list)
{
    if (row_list.empty()) return true;
    if (!is_enabled) return false;
    try
    {
        map<string, map<string, vector<Mutation> > > mutation_map;
        time_t timestamp = createTimeStamp();
        for (vector<PriceHistory>::const_iterator vit = row_list.begin();
                vit != row_list.end(); ++vit)
        {
            vector<Mutation>& mutation_list = mutation_map[vit->docId_][cf_name];
            for (PriceHistoryType::const_iterator mit = vit->priceHistory_.begin();
                    mit != vit->priceHistory_.end(); ++mit)
            {
                mutation_list.push_back(Mutation());
                Mutation& mut = mutation_list.back();
                mut.__isset.column_or_supercolumn = true;
                mut.column_or_supercolumn.__isset.column = true;
                Column& col = mut.column_or_supercolumn.column;
                col.__set_name(serializeLong(mit->first));
                col.__set_value(toBytes(mit->second));
                col.__set_timestamp(timestamp);
                col.__set_ttl(63072000);
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

bool PriceHistory::getMultiSlice(
        vector<PriceHistory>& row_list,
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
        if (raw_column_map.empty()) return true;

        for (map<string, vector<ColumnOrSuperColumn> >::const_iterator mit = raw_column_map.begin();
                mit != raw_column_map.end(); ++mit)
        {
            if (mit->second.empty()) continue;
            row_list.push_back(PriceHistory(mit->first));
            PriceHistory& price_history = row_list.back();
            for (vector<ColumnOrSuperColumn>::const_iterator vit = mit->second.begin();
                    vit != mit->second.end(); ++vit)
            {
                price_history.insert(vit->column.name, vit->column.value);
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

bool PriceHistory::getMultiCount(
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
        cout << ire.why << endl;
        return false;
    }
    return true;
}

bool PriceHistory::updateRow() const
{
    if (!is_enabled || docId_.empty()) return false;
    if (!priceHistoryPresent_) return true;
    try
    {
        for (PriceHistoryType::const_iterator it = priceHistory_.begin();
                it != priceHistory_.end(); ++it)
        {
            CassandraConnection::instance().getCassandraClient()->insertColumn(
                    toBytes(it->second),
                    docId_,
                    cf_name,
                    serializeLong(it->first),
                    createTimeStamp(),
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

bool PriceHistory::insert(const string& name, const string& value)
{
    clear();
    if (value.length() != sizeof(ProductPrice))
    {
        cerr << "Bad insert!" << endl;
        return false;
    }
    priceHistory_[deserializeLong(name)] = fromBytes<ProductPrice>(value);
    return true;
}

void PriceHistory::insert(time_t timestamp, ProductPrice price)
{
    clear();
    priceHistory_[timestamp] = price;
}

void PriceHistory::resetKey(const string& newDocId)
{
    if (newDocId.empty())
    {
        docId_.clear();
        priceHistoryPresent_ = false;
    }
    else
        docId_.assign(newDocId);
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
