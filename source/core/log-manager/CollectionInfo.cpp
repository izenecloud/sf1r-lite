#include "CollectionInfo.h"

#include <libcassandra/cassandra.h>

#include <boost/assign/list_of.hpp>
#include <boost/algorithm/string.hpp>

using namespace std;
using namespace libcassandra;
using namespace org::apache::cassandra;
using namespace boost;
using namespace boost::assign;
using namespace boost::posix_time;
using namespace boost::algorithm;

namespace sf1r {

const string CollectionInfo::cassandra_name("CollectionInfo");

const string CollectionInfo::cassandra_column_type("Super");

const string CollectionInfo::cassandra_comparator_type;

const string CollectionInfo::cassandra_sub_comparator_type;

const string CollectionInfo::cassandra_comment;

const double CollectionInfo::cassandra_row_cache_size(0);

const double CollectionInfo::cassandra_key_cache_size(200000);

const double CollectionInfo::cassandra_read_repair_chance(1);

const vector<ColumnDefFake> CollectionInfo::cassandra_column_metadata;

const int32_t CollectionInfo::cassandra_gc_grace_seconds(864000);

const string CollectionInfo::cassandra_default_validation_class;

const int32_t CollectionInfo::cassandra_id(0);

const int32_t CollectionInfo::cassandra_min_compaction_threshold(4);

const int32_t CollectionInfo::cassandra_max_compaction_threshold(22);

const int32_t CollectionInfo::cassandra_row_cache_save_period_in_seconds(0);

const int32_t CollectionInfo::cassandra_key_cache_save_period_in_seconds(0);

const map<string, string> CollectionInfo::cassandra_compression_options
    = map_list_of("sstable_compression", "SnappyCompressor")("chunk_length_kb", "64");

const string CollectionInfo::SuperColumns[] = {"SourceCount"};

CollectionInfo::CollectionInfo(const std::string& collection)
    : CassandraColumnFamily()
    , collection_(collection)
    , collectionPresent_(!collection.empty())
    , sourceCountPresent_(false)
{}

CollectionInfo::~CollectionInfo()
{}

bool CollectionInfo::updateRow() const
{
    if (!collectionPresent_) return false;
    try
    {
        for (SourceCountType::const_iterator it = sourceCount_.begin();
                it != sourceCount_.end(); ++it)
        {
            CassandraConnection::instance().getCassandraClient()->insertColumn(
                    join(list_of(lexical_cast<string>(it->second.get<0>()))
                                (it->second.get<1>())
                                (it->second.get<2>()), ":"),
                    collection_,
                    cassandra_name,
                    SuperColumns[0],
                    to_iso_string(it->first));
        }
    }
    catch (InvalidRequestException& ire)
    {
        cerr << ire.why << endl;
        return false;
    }
    return true;
}

bool CollectionInfo::deleteRow()
{
    if (!collectionPresent_) return false;
    try
    {
        ColumnPath col_path;
        col_path.__set_column_family(cassandra_name);
        CassandraConnection::instance().getCassandraClient()->remove(
                collection_,
                col_path);
        reset();
    }
    catch (InvalidRequestException &ire)
    {
        cout << ire.why << endl;
        return false;
    }
    return true;
}

bool CollectionInfo::getRow()
{
    if (!collectionPresent_) return false;
    try
    {
        SuperColumn super_column;
        CassandraConnection::instance().getCassandraClient()->getSuperColumn(
                super_column,
                collection_,
                cassandra_name,
                SuperColumns[0]);
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
            sourceCount_[from_iso_string(it->name)] = SourceCountItemType(lexical_cast<uint32_t>(tokens[0]), tokens[1], tokens[2]);
        }
    }
    catch (InvalidRequestException &ire)
    {
        cout << ire.why << endl;
        return false;
    }
    return true;
}

void CollectionInfo::insertSourceCount(ptime timeStamp, const SourceCountItemType& sourceCountItem)
{
    if (!sourceCountPresent_)
    {
        sourceCount_.clear();
        sourceCountPresent_ = true;
    }
    sourceCount_[timeStamp] = sourceCountItem;
}

void CollectionInfo::reset(const string& newCollection)
{
    if (!newCollection.empty())
        collection_.assign(newCollection);
    sourceCountPresent_ = false;
}

}
