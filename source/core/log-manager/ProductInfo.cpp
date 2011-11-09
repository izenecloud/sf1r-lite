/**
 *file ProductInfo.cpp
 *brief
 *date Sep 9, 2011
 *author Xin Liu
 */

#include "ProductInfo.h"

#include <boost/assign/list_of.hpp>

using namespace std;
using namespace boost::assign;
using namespace org::apache::cassandra;

namespace sf1r {

const string ProductInfo::cassandra_name("ProductInfo");

const string ProductInfo::cassandra_column_type("Super");

const string ProductInfo::cassandra_comparator_type;

const string ProductInfo::cassandra_sub_comparator_type;

const string ProductInfo::cassandra_comment;

const double ProductInfo::cassandra_row_cache_size(0);

const double ProductInfo::cassandra_key_cache_size(200000);

const double ProductInfo::cassandra_read_repair_chance(1);

const vector<ColumnDef> ProductInfo::cassandra_column_metadata;

const int32_t ProductInfo::cassandra_gc_grace_seconds(864000);

const string ProductInfo::cassandra_default_validation_class;

const int32_t ProductInfo::cassandra_id(CassandraConnection::PRODUCT_INFO);

const int32_t ProductInfo::cassandra_min_compaction_threshold(4);

const int32_t ProductInfo::cassandra_max_compaction_threshold(22);

const int32_t ProductInfo::cassandra_row_cache_save_period_in_seconds(0);

const int32_t ProductInfo::cassandra_key_cache_save_period_in_seconds(0);

const map<string, string> ProductInfo::cassandra_compression_options
    = map_list_of("sstable_compression", "SnappyCompressor")("chunk_length_kb", "64");

}
