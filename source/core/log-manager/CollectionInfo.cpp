#include "CollectionInfo.h"

#include <libcassandra/cassandra.h>

#include <boost/assign/list_of.hpp>

using namespace std;
using namespace libcassandra;
using namespace org::apache::cassandra;
using namespace boost::assign;
using namespace boost::posix_time;

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

CollectionInfo::CollectionInfo(const std::string& collection)
    : CassandraColumnFamily()
    , collection_(collection)
    , collectionPresent_(!collection.empty())
    , sourcePresent_(false)
    , countPresent_(false)
    , flagPresent_(false)
{}

CollectionInfo::~CollectionInfo()
{}

bool CollectionInfo::updateRow() const
{
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
}

bool CollectionInfo::save()
{
    if (!hasCollection())
        return false;

    if (!hasTimeStamp())
        timeStamp_ = microsec_clock::local_time();
    string time_string = to_iso_string(timeStamp_);

    boost::shared_ptr<Cassandra> client(CassandraConnection::instance().getCassandraClient());
    try
    {
        if (hasSource())
        {
            client->insertColumn(
                    source_,
                    collection_,
                    cassandra_name,
                    time_string,
                    "Source");
        }
        if (hasCount())
        {
            client->insertColumn(
                    count_,
                    collection_,
                    cassandra_name,
                    time_string,
                    "Count");
        }
        if (hasFlag())
        {
            client->insertColumn(
                    flag_,
                    collection_,
                    cassandra_name,
                    time_string,
                    "Flag");
        }
    }
    catch (InvalidRequestException& ire)
    {
        cerr << ire.why << endl;
        return false;
    }
    return true;
}

void CollectionInfo::reset(const string& newCollection)
{
    if (!newCollection.empty())
        collection_.assign(newCollection);
    sourcePresent_ = countPresent_ = flagPresent_ = timeStampPresent_ = false;
}

}
