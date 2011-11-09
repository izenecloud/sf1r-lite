#ifndef _CASSANDRA_COLUMN_FAMILY_H_
#define _CASSANDRA_COLUMN_FAMILY_H_

#include "CassandraConnection.h"

namespace sf1r {

class CassandraColumnFamily
{

public:
    CassandraColumnFamily() : exist_(false)
    {}

    virtual ~CassandraColumnFamily() {}

protected:
    bool exist_;
};

#define DECLARE_COLUMN_FAMILY_COMMON_ROUTINES \
public: \
    static const std::string name; \
    static const std::string column_type; \
    static const std::string comparator_type; \
    static const std::string sub_comparator_type; \
    static const std::string comment; \
    static const double row_cache_size; \
    static const double key_cache_size; \
    static const double read_repair_chance; \
    static const std::vector<org::apache::cassandra::ColumnDef> column_metadata; \
    static const int32_t gc_grace_seconds; \
    static const std::string default_validation_class; \
    static const int32_t id; \
    static const int32_t min_compaction_threshold; \
    static const int32_t max_compaction_threshold; \
    static const int32_t row_cache_save_period_in_seconds; \
    static const int32_t key_cache_save_period_in_seconds; \
    static const std::map<std::string, std::string> compression_options; \
    \
    static void createColumnFamily() \
    { \
        libcassandra::ColumnFamilyDefinition definition( \
            CassandraConnection::instance().getKeyspaceName(), \
            name, \
            column_type, \
            comparator_type, \
            sub_comparator_type, \
            comment, \
            row_cache_size, \
            key_cache_size, \
            read_repair_chance, \
            column_metadata, \
            gc_grace_seconds, \
            default_validation_class, \
            id, \
            min_compaction_threshold, \
            max_compaction_threshold, \
            row_cache_save_period_in_seconds, \
            key_cache_save_period_in_seconds, \
            compression_options); \
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
            {} \
        } \
        catch (...) \
        {} \
    } \


}

#endif
