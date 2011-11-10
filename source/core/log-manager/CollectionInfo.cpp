#include "CollectionInfo.h"

#include <boost/assign/list_of.hpp>

using namespace std;
using namespace boost::assign;
using namespace org::apache::cassandra;
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

const vector<ColumnDef> CollectionInfo::cassandra_column_metadata;

const int32_t CollectionInfo::cassandra_gc_grace_seconds(864000);

const string CollectionInfo::cassandra_default_validation_class;

const int32_t CollectionInfo::cassandra_id(CassandraConnection::COLLECTION_INFO);

const int32_t CollectionInfo::cassandra_min_compaction_threshold(4);

const int32_t CollectionInfo::cassandra_max_compaction_threshold(22);

const int32_t CollectionInfo::cassandra_row_cache_save_period_in_seconds(0);

const int32_t CollectionInfo::cassandra_key_cache_save_period_in_seconds(0);

const map<string, string> CollectionInfo::cassandra_compression_options
    = map_list_of("sstable_compression", "SnappyCompressor")("chunk_length_kb", "64");

void CollectionInfo::save()
{
    if (!hasCollection())
        return;

    if (!hasTimeStamp())
        timeStamp_ = microsec_clock::local_time();
    string time_string = to_iso_string(timeStamp_);

    if (hasSource())
    {
        cassandraClient()->insertColumn(
                source_,
                collection_,
                cassandra_name,
                time_string,
                "Source");
    }

    if (hasNum())
    {
        cassandraClient()->insertColumn(
                num_,
                collection_,
                cassandra_name,
                time_string,
                "Num");
    }

    if (hasFlag())
    {
        cassandraClient()->insertColumn(
                flag_,
                collection_,
                cassandra_name,
                time_string,
                "Flag");
    }
}

}
