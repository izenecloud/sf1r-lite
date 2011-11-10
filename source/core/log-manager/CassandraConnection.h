#ifndef _CASSANDRA_CONNECTION_H_
#define _CASSANDRA_CONNECTION_H_

#include <string>
#include <map>
#include <list>
#include <vector>

#include <util/ThreadModel.h>

#include "LogManagerSingleton.h"

namespace libcassandra {
class Cassandra;
}

namespace sf1r {

class ColumnDefFake
{
public:
    enum IndexType {
        KEYS = 0,
        CUSTOM = 1,
        UNSET = 2
    };

    ColumnDefFake() : index_type_(UNSET)
    {}

    ColumnDefFake(
            const std::string& name,
            const std::string& validation_class,
            const IndexType index_type,
            const std::string& index_name,
            const std::map<std::string, std::string>& index_options)
        : name_(name)
        , validation_class_(validation_class)
        , index_type_(index_type)
        , index_name_(index_name)
        , index_options_(index_options)
    {}

    ~ColumnDefFake() {}

    std::string name_;
    std::string validation_class_;
    IndexType index_type_;
    std::string index_name_;
    std::map<std::string, std::string> index_options_;

};

class CassandraConnection : public LogManagerSingleton<CassandraConnection>
{
public:
    CassandraConnection();

    ~CassandraConnection();

    const std::string& getKeyspaceName() const;

    void setKeyspaceName(const std::string& keyspace_name);

    bool init(const std::string& str);

    boost::shared_ptr<libcassandra::Cassandra>& getCassandraClient()
    {
        return cassandra_client_;
    }

    bool createColumnFamily(
            const std::string& in_name,
            const std::string& in_column_type,
            const std::string& in_comparator_type,
            const std::string& in_sub_comparator_type,
            const std::string& in_comment,
            const double in_row_cache_size,
            const double in_key_cache_size,
            const double in_read_repair_chance,
            const std::vector<ColumnDefFake>& in_column_metadata,
            const int32_t in_gc_grace_seconds,
            const std::string& in_default_validation_class,
            const int32_t in_id,
            const int32_t in_min_compaction_threshold,
            const int32_t in_max_compaction_threshold,
            const int32_t in_row_cache_save_period_in_seconds,
            const int32_t in_key_cache_save_period_in_seconds,
            const std::map<std::string, std::string>& in_compression_options);

    bool truncateColumnFamily(const std::string& in_name);

    bool dropColumnFamily(const std::string& in_name);

public:
    enum COLUMN_FAMILY_ID {
        PRODUCT_INFO = 2000,
        COLLECTION_INFO,
        CUSTOMER_INFO,
        VENDOR_INFO,
        SYSTEM_INFO = 9999 //XXX Please add new entries right before this line
    };

private:
    std::string keyspace_name_;

    boost::shared_ptr<libcassandra::Cassandra> cassandra_client_;
};

}

#endif
