#ifndef _CASSANDRA_COLUMN_FAMILY_H_
#define _CASSANDRA_COLUMN_FAMILY_H_

#include "CassandraConnection.h"

#include <boost/date_time/posix_time/posix_time.hpp>

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

template <typename ColumnFamilyType>
bool createColumnFamily()
{
    return CassandraConnection::instance().createColumnFamily(
            ColumnFamilyType::cassandra_name,
            ColumnFamilyType::cassandra_column_type,
            ColumnFamilyType::cassandra_comparator_type,
            ColumnFamilyType::cassandra_sub_comparator_type,
            ColumnFamilyType::cassandra_comment,
            ColumnFamilyType::cassandra_row_cache_size,
            ColumnFamilyType::cassandra_key_cache_size,
            ColumnFamilyType::cassandra_read_repair_chance,
            ColumnFamilyType::cassandra_column_metadata,
            ColumnFamilyType::cassandra_gc_grace_seconds,
            ColumnFamilyType::cassandra_default_validation_class,
            ColumnFamilyType::cassandra_id,
            ColumnFamilyType::cassandra_min_compaction_threshold,
            ColumnFamilyType::cassandra_max_compaction_threshold,
            ColumnFamilyType::cassandra_row_cache_save_period_in_seconds,
            ColumnFamilyType::cassandra_key_cache_save_period_in_seconds,
            ColumnFamilyType::cassandra_compression_options);
}

template <typename ColumnFamilyType>
bool truncateColumnFamily()
{
    return CassandraConnection::instance().truncateColumnFamily(ColumnFamilyType::cassandra_name);
}

template <typename ColumnFamilyType>
bool dropColumnFamily()
{
    return CassandraConnection::instance().dropColumnFamily(ColumnFamilyType::cassandra_name);
}

#define DEFINE_COLUMN_FAMILY_COMMON_ROUTINES(ClassName) \
public: \
    static const std::string cassandra_name; \
    static const std::string cassandra_column_type; \
    static const std::string cassandra_comparator_type; \
    static const std::string cassandra_sub_comparator_type; \
    static const std::string cassandra_comment; \
    static const double cassandra_row_cache_size; \
    static const double cassandra_key_cache_size; \
    static const double cassandra_read_repair_chance; \
    static const std::vector<ColumnDefFake> cassandra_column_metadata; \
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
        return ::sf1r::createColumnFamily<ClassName>(); \
    } \
    \
    static bool truncateColumnFamily() \
    { \
        return ::sf1r::truncateColumnFamily<ClassName>(); \
    } \
    \
    static bool dropColumnFamily() \
    { \
        return ::sf1r::dropColumnFamily<ClassName>(); \
    } \


}

#endif
