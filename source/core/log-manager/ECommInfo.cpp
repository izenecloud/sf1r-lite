#include "ECommInfo.h"

#include <libcassandra/util_functions.h>

#include <boost/assign/list_of.hpp>

using namespace std;
using namespace boost::assign;
using namespace org::apache::cassandra;
using namespace libcassandra;

namespace sf1r {

const string ECommInfo::cassandra_name("ECommInfo");

const string ECommInfo::cassandra_column_type("Super");

const string ECommInfo::cassandra_comparator_type("LongType");

const string ECommInfo::cassandra_sub_comparator_type;

const string ECommInfo::cassandra_comment;

const double ECommInfo::cassandra_row_cache_size(0);

const double ECommInfo::cassandra_key_cache_size(200000);

const double ECommInfo::cassandra_read_repair_chance(1);

const vector<ColumnDef> ECommInfo::cassandra_column_metadata;

const int32_t ECommInfo::cassandra_gc_grace_seconds(864000);

const string ECommInfo::cassandra_default_validation_class;

const int32_t ECommInfo::cassandra_id(CassandraConnection::ECOMM_INFO);

const int32_t ECommInfo::cassandra_min_compaction_threshold(4);

const int32_t ECommInfo::cassandra_max_compaction_threshold(22);

const int32_t ECommInfo::cassandra_row_cache_save_period_in_seconds(0);

const int32_t ECommInfo::cassandra_key_cache_save_period_in_seconds(0);

const map<string, string> ECommInfo::cassandra_compression_options
    = map_list_of("sstable_compression", "SnappyCompressor")("chunk_length_kb", "64");

void ECommInfo::save()
{
    string time_string = serializeLong(timeStamp_);
    cassandraClient()->insertColumn(
            collection_,
            source_,
            cassandra_name,
            time_string,
            "Collection"
            );
    cassandraClient()->insertColumn(
            num_,
            source_,
            cassandra_name,
            time_string,
            "Num"
            );
    cassandraClient()->insertColumn(
            flag_,
            source_,
            cassandra_name,
            time_string,
            "Flag"
            );
}

}
