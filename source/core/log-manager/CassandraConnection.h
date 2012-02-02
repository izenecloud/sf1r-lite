#ifndef _CASSANDRA_CONNECTION_H_
#define _CASSANDRA_CONNECTION_H_

#include <map>
#include <list>
#include <vector>
#include <iostream>
#include <boost/thread/shared_mutex.hpp>

#include <util/ThreadModel.h>

#include "LogManagerSingleton.h"

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

    bool init(const std::string& str);

    bool isEnabled();

    libcassandra::Cassandra* getCassandraClient(const std::string& keyspace_name);

    /**
     * drop a keyspace.
     * @return true for success, false for error or no keyspace found
     * @attention after this function is called, the @c libcassandra::Cassandra* returned by
     *            @c getCassandraClient() previously with the same keyspace name is invalid,
     *            if you need to create the keyspace again, you have to call @c getCassandraClient()
     *            again to fetch a new @c libcassandra::Cassandra*.
     */
    bool dropKeyspace(const std::string& keyspace_name);

    bool createColumnFamily(
            const std::string& in_keyspace_name,
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
    void clear_();

    libcassandra::Cassandra* getClient_(const std::string& keyspace_name);
    libcassandra::Cassandra* createClient_(const std::string& keyspace_name);
    bool initClient_(libcassandra::Cassandra* client, const std::string& keyspace_name);

private:
    bool isEnabled_;

    std::string user_name_;
    std::string password_;

    /** keyspace name => client instance */
    typedef std::map<std::string, libcassandra::Cassandra*> KeyspaceClientMap;
    KeyspaceClientMap client_map_;

    boost::shared_mutex keyspace_mutex_;
};

#define CATCH_CASSANDRA_EXCEPTION(prompt) \
    catch (const org::apache::cassandra::InvalidRequestException& ire) \
    { \
        std::cerr << prompt << " [" << ire.why << "]" << std::endl; \
        return false; \
    } \
    catch (const ::apache::thrift::TException& tex) \
    { \
        std::cerr << prompt << " [" << tex.what() << "]" << std::endl; \
        return false; \
    } \
    catch (const std::exception& ex) \
    { \
        std::cerr << prompt << " [" << ex.what() << "]" << std::endl; \
        return false; \
    } \
    catch (...) \
    { \
        std::cerr << prompt << " [Unknown error]" << std::endl; \
        return false; \
    } \


}

#endif
