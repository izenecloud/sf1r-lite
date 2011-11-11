#include "CollectionInfo.h"

#include <libcassandra/cassandra.h>
#include <libcassandra/util_functions.h>

#include <boost/assign/list_of.hpp>
#include <boost/algorithm/string.hpp>

using namespace std;
using namespace libcassandra;
using namespace org::apache::cassandra;
using namespace boost;
using namespace boost::assign;
using namespace boost::algorithm;

namespace sf1r {

const string CollectionInfo::cf_name("CollectionInfo");

const string CollectionInfo::cf_column_type("Super");

const string CollectionInfo::cf_comparator_type;

const string CollectionInfo::cf_sub_comparator_type;

const string CollectionInfo::cf_comment;

const double CollectionInfo::cf_row_cache_size(0);

const double CollectionInfo::cf_key_cache_size(200000);

const double CollectionInfo::cf_read_repair_chance(1);

const vector<ColumnDef> CollectionInfo::cf_column_metadata;

const int32_t CollectionInfo::cf_gc_grace_seconds(864000);

const string CollectionInfo::cf_default_validation_class;

const int32_t CollectionInfo::cf_id(0);

const int32_t CollectionInfo::cf_min_compaction_threshold(4);

const int32_t CollectionInfo::cf_max_compaction_threshold(22);

const int32_t CollectionInfo::cf_row_cache_save_period_in_seconds(0);

const int32_t CollectionInfo::cf_key_cache_save_period_in_seconds(0);

const map<string, string> CollectionInfo::cf_compression_options
    = map_list_of("sstable_compression", "SnappyCompressor")("chunk_length_kb", "64");

const string CollectionInfo::SuperColumnName[] = {"SourceCount"};

CollectionInfo::CollectionInfo(const std::string& collection)
    : ColumnFamilyBase()
    , collection_(collection)
    , collectionPresent_(!collection.empty())
    , sourceCountPresent_(false)
{}

CollectionInfo::~CollectionInfo()
{}

bool CollectionInfo::updateRow() const
{
    if (!CassandraConnection::instance().isEnabled() || !collectionPresent_) return false;
    try
    {
        for (SourceCountType::const_iterator it = sourceCount_.begin();
                it != sourceCount_.end(); ++it)
        {
            CassandraConnection::instance().getCassandraClient()->insertColumn(
                    lexical_cast<string>(it->second.get<0>())
                        + ":" + it->second.get<1>()
                        + ":" + it->second.get<2>(),
                    collection_,
                    cf_name,
                    SuperColumnName[0],
                    serializeLong(it->first));
        }
    }
    catch (const InvalidRequestException& ire)
    {
        cerr << ire.why << endl;
        return false;
    }
    return true;
}

bool CollectionInfo::deleteRow()
{
    if (!CassandraConnection::instance().isEnabled() || !collectionPresent_) return false;
    try
    {
        ColumnPath col_path;
        col_path.__set_column_family(cf_name);
        CassandraConnection::instance().getCassandraClient()->remove(
                collection_,
                col_path);
        resetKey();
    }
    catch (const InvalidRequestException &ire)
    {
        cout << ire.why << endl;
        return false;
    }
    return true;
}

bool CollectionInfo::getRow()
{
    if (!CassandraConnection::instance().isEnabled() || !collectionPresent_) return false;
    try
    {
        SuperColumn super_column;
        CassandraConnection::instance().getCassandraClient()->getSuperColumn(
                super_column,
                collection_,
                cf_name,
                SuperColumnName[0]);
        if (super_column.columns.empty())
            return true;
        if (!sourceCountPresent_)
        {
            sourceCount_.clear();
            sourceCountPresent_ = true;
        }
        for (vector<Column>::const_iterator it = super_column.columns.begin();
                it != super_column.columns.end(); ++it)
        {
            vector<string> tokens(3);
            split(tokens, it->value, is_any_of(":"));
            sourceCount_[deserializeLong(it->name)] = SourceCountItemType(lexical_cast<uint32_t>(tokens[0]), tokens[1], tokens[2]);
        }
    }
    catch (const InvalidRequestException &ire)
    {
        cout << ire.why << endl;
        return false;
    }
    return true;
}

void CollectionInfo::insertSourceCount(const SourceCountItemType& sourceCountItem)
{
    if (!sourceCountPresent_)
    {
        sourceCount_.clear();
        sourceCountPresent_ = true;
    }
    sourceCount_[createTimestamp()] = sourceCountItem;
}

void CollectionInfo::insertSourceCount(time_t timeStamp, const SourceCountItemType& sourceCountItem)
{
    if (!sourceCountPresent_)
    {
        sourceCount_.clear();
        sourceCountPresent_ = true;
    }
    sourceCount_[timeStamp] = sourceCountItem;
}

void CollectionInfo::resetKey(const string& newCollection)
{
    if (!newCollection.empty())
        collection_.assign(newCollection);
    sourceCountPresent_ = false;
}

}
