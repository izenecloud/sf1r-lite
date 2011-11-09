/**
 *file ProductInfo.cpp
 *brief
 *date Sep 9, 2011
 *author Xin Liu
 */

#include "ProductInfo.h"

#include <libcassandra/cassandra.h>
#include <libcassandra/column_family_definition.h>

#include <boost/assign/list_of.hpp>

using namespace std;
using namespace boost::assign;
using namespace org::apache::cassandra;

namespace sf1r {

//XXX Below are configurations for column family. Don't omit any one of them!

const string ProductInfo::name("ProductInfo");

const string ProductInfo::column_type("Super");

const string ProductInfo::comparator_type;

const string ProductInfo::sub_comparator_type;

const string ProductInfo::comment;

const double ProductInfo::row_cache_size(0);

const double ProductInfo::key_cache_size(200000);

const double ProductInfo::read_repair_chance(1);

const vector<ColumnDef> ProductInfo::column_metadata;

const int32_t ProductInfo::gc_grace_seconds(864000);

const string ProductInfo::default_validation_class;

const int32_t ProductInfo::id(CassandraConnection::PRODUCT_INFO);

const int32_t ProductInfo::min_compaction_threshold(4);

const int32_t ProductInfo::max_compaction_threshold(22);

const int32_t ProductInfo::row_cache_save_period_in_seconds(0);

const int32_t ProductInfo::key_cache_save_period_in_seconds(0);

const map<string, string> ProductInfo::compression_options = map_list_of("sstable_compression", "SnappyCompressor")("chunk_length_kb", "64");

}
