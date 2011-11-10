#ifndef _CASSANDRA_COLUMN_FAMILY_H_
#define _CASSANDRA_COLUMN_FAMILY_H_

#include "CassandraConnection.h"

#include <libcassandra/genthrift/cassandra_types.h>
#include <libcassandra/cassandra.h>

namespace sf1r {

class CassandraColumnFamily
{

public:
    CassandraColumnFamily() : exist_(false)
    {}

    virtual ~CassandraColumnFamily() {}

    static boost::shared_ptr<libcassandra::Cassandra>& cassandraClient()
    {
        return CassandraConnection::instance().getCassandraClient();
    }

protected:
    bool exist_;
};

#define DECLARE_COLUMN_FAMILY_COMMON_ROUTINES \
public: \
    static const std::string cassandra_name; \
    static const std::string cassandra_column_type; \
    static const std::string cassandra_comparator_type; \
    static const std::string cassandra_sub_comparator_type; \
    static const std::string cassandra_comment; \
    static const double cassandra_row_cache_size; \
    static const double cassandra_key_cache_size; \
    static const double cassandra_read_repair_chance; \
    static const std::vector<org::apache::cassandra::ColumnDef> cassandra_column_metadata; \
    static const int32_t cassandra_gc_grace_seconds; \
    static const std::string cassandra_default_validation_class; \
    static const int32_t cassandra_id; \
    static const int32_t cassandra_min_compaction_threshold; \
    static const int32_t cassandra_max_compaction_threshold; \
    static const int32_t cassandra_row_cache_save_period_in_seconds; \
    static const int32_t cassandra_key_cache_save_period_in_seconds; \
    static const std::map<std::string, std::string> cassandra_compression_options; \
    \
    static bool createColumnFamily() \
    { \
        libcassandra::ColumnFamilyDefinition definition( \
            CassandraConnection::instance().getKeyspaceName(), \
            cassandra_name, \
            cassandra_column_type, \
            cassandra_comparator_type, \
            cassandra_sub_comparator_type, \
            cassandra_comment, \
            cassandra_row_cache_size, \
            cassandra_key_cache_size, \
            cassandra_read_repair_chance, \
            cassandra_column_metadata, \
            cassandra_gc_grace_seconds, \
            cassandra_default_validation_class, \
            cassandra_id, \
            cassandra_min_compaction_threshold, \
            cassandra_max_compaction_threshold, \
            cassandra_row_cache_save_period_in_seconds, \
            cassandra_key_cache_save_period_in_seconds, \
            cassandra_compression_options); \
        \
        try \
        { \
            CassandraConnection::instance().getCassandraClient()->createColumnFamily(definition); \
        } \
        catch (org::apache::cassandra::InvalidRequestException &ire) \
        { \
            try \
            { \
                CassandraConnection::instance().getCassandraClient()->updateColumnFamily(definition); \
            } \
            catch (...) \
            { \
                return false; \
            } \
        } \
        catch (...) \
        { \
            return false; \
        } \
        return true; \
    } \


}

#endif
