#ifndef _CASSANDRA_CONNECTION_H_
#define _CASSANDRA_CONNECTION_H_

#include <map>
#include <list>
#include <vector>
#include <iostream>

#include <util/ThreadModel.h>

#include "LogManagerSingleton.h"
#include "UtilFunctions.h"

namespace libcassandra
{
class Cassandra;
}

namespace org
{
namespace apache
{
namespace cassandra
{
class ColumnDef;
}
}
}

namespace sf1r {

class CassandraConnection : public LogManagerSingleton<CassandraConnection>
{
public:
    CassandraConnection();

    ~CassandraConnection();

    const std::string& getKeyspaceName() const;

    void setKeyspaceName(const std::string& keyspace_name);

    bool init(const std::string& str);

    bool isEnabled();

    boost::shared_ptr<libcassandra::Cassandra>& getCassandraClient();

    int64_t createTimestamp();

    bool createColumnFamily(
            const std::string& in_name,
            const std::string& in_column_type,
            const std::string& in_comparator_type,
            const std::string& in_sub_comparator_type,
            const std::string& in_comment,
            const double in_row_cache_size,
            const double in_key_cache_size,
            const double in_read_repair_chance,
            const std::vector<org::apache::cassandra::ColumnDef>& in_column_metadata,
            const int32_t in_gc_grace_seconds,
            const std::string& in_default_validation_class,
            const int32_t in_id,
            const int32_t in_min_compaction_threshold,
            const int32_t in_max_compaction_threshold,
            const int32_t in_row_cache_save_period_in_seconds,
            const int32_t in_key_cache_save_period_in_seconds,
            const int8_t in_replicate_on_write,
            const double in_merge_shards_chance,
            const std::string& in_key_validation_class,
            const std::string& in_row_cache_provider,
            const std::string& in_key_alias,
            const std::string& in_compaction_strategy,
            const std::map<std::string, std::string>& in_compaction_strategy_options,
            const int32_t in_row_cache_keys_to_save,
            const std::map<std::string, std::string>& in_compression_options);

private:
    bool isEnabled_;

    std::string keyspace_name_;

    boost::shared_ptr<libcassandra::Cassandra> cassandra_client_;
};

}

#endif
