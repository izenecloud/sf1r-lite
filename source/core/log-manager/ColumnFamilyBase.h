#ifndef _COLUMN_FAMILY_BASE_H_
#define _COLUMN_FAMILY_BASE_H_

#include "CassandraConnection.h"

#include <iostream>

namespace sf1r {

class ColumnFamilyBase
{

public:
    ColumnFamilyBase() : exist_(false)
    {}

    virtual ~ColumnFamilyBase() {}

    virtual const std::string& getKey() const = 0;

    virtual const std::string& getName() const = 0;

    virtual bool updateRow() const = 0;

    virtual bool getRow() = 0;

    virtual bool deleteRow();

    virtual bool getCount(int32_t& count, const std::string& start, const std::string& finish) const;

    virtual void resetKey(const std::string& newKey = "") = 0;

protected:
    bool exist_;
};

template <typename ColumnFamilyType>
const std::string& getColumnFamilyName()
{
    return ColumnFamilyType::cf_name;
}

template <typename ColumnFamilyType>
bool createColumnFamily()
{
    return CassandraConnection::instance().createColumnFamily(
            ColumnFamilyType::cf_name,
            ColumnFamilyType::cf_column_type,
            ColumnFamilyType::cf_comparator_type,
            ColumnFamilyType::cf_sub_comparator_type,
            ColumnFamilyType::cf_comment,
            ColumnFamilyType::cf_row_cache_size,
            ColumnFamilyType::cf_key_cache_size,
            ColumnFamilyType::cf_read_repair_chance,
            ColumnFamilyType::cf_column_metadata,
            ColumnFamilyType::cf_gc_grace_seconds,
            ColumnFamilyType::cf_default_validation_class,
            ColumnFamilyType::cf_id,
            ColumnFamilyType::cf_min_compaction_threshold,
            ColumnFamilyType::cf_max_compaction_threshold,
            ColumnFamilyType::cf_row_cache_save_period_in_seconds,
            ColumnFamilyType::cf_key_cache_save_period_in_seconds,
            ColumnFamilyType::cf_replicate_on_write,
            ColumnFamilyType::cf_merge_shards_chance,
            ColumnFamilyType::cf_key_validation_class,
            ColumnFamilyType::cf_row_cache_provider,
            ColumnFamilyType::cf_key_alias,
            ColumnFamilyType::cf_compaction_strategy,
            ColumnFamilyType::cf_compaction_strategy_options,
            ColumnFamilyType::cf_row_cache_keys_to_save,
            ColumnFamilyType::cf_compression_options);
}

template <typename ColumnFamilyType>
bool truncateColumnFamily()
{
    return CassandraConnection::instance().truncateColumnFamily(ColumnFamilyType::cf_name);
}

template <typename ColumnFamilyType>
bool dropColumnFamily()
{
    return CassandraConnection::instance().dropColumnFamily(ColumnFamilyType::cf_name);
}

template <typename ColumnFamilyType>
bool getSingleRow(ColumnFamilyType& row, const std::string& key)
{
    row.resetKey(key);
    return row.getRow();
}

template <typename ColumnFamilyType>
bool getMultiRow(std::vector<ColumnFamilyType>& row_list, const std::vector<std::string>& key_list)
{
    for (std::vector<std::string>::const_iterator it = key_list.begin();
            it != key_list.end(); ++it)
    {
        row_list.push_back(ColumnFamilyType(*it));
        if (!row_list.back().getRow())
            return false;
    }
    return true;
}

template <typename T>
std::string toBytes(const T& val)
{
    return std::string(reinterpret_cast<const char *>(&val), sizeof(T));
}

template <typename T>
T fromBytes(const std::string& str)
{
    return *(reinterpret_cast<const T *>(str.c_str()));
}

#define DEFINE_COLUMN_FAMILY_COMMON_ROUTINES(ClassName) \
public: \
    static const std::string cf_name; \
    static const std::string cf_column_type; \
    static const std::string cf_comparator_type; \
    static const std::string cf_sub_comparator_type; \
    static const std::string cf_comment; \
    static const double cf_row_cache_size; \
    static const double cf_key_cache_size; \
    static const double cf_read_repair_chance; \
    static const std::vector<org::apache::cassandra::ColumnDef> cf_column_metadata; \
    static const int32_t cf_gc_grace_seconds; \
    static const std::string cf_default_validation_class; \
    static const int32_t cf_id; \
    static const int32_t cf_min_compaction_threshold; \
    static const int32_t cf_max_compaction_threshold; \
    static const int32_t cf_row_cache_save_period_in_seconds; \
    static const int32_t cf_key_cache_save_period_in_seconds; \
    static const int8_t cf_replicate_on_write; \
    static const double cf_merge_shards_chance; \
    static const std::string cf_key_validation_class; \
    static const std::string cf_row_cache_provider; \
    static const std::string cf_key_alias; \
    static const std::string cf_compaction_strategy; \
    static const std::map<std::string, std::string> cf_compaction_strategy_options; \
    static const int32_t cf_row_cache_keys_to_save; \
    static const std::map<std::string, std::string> cf_compression_options; \
    \
    const std::string& getName() const \
    { \
        return ::sf1r::getColumnFamilyName<ClassName>(); \
    } \
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
    \
    static bool getSingleRow(ClassName& row, const std::string& key) \
    { \
        return ::sf1r::getSingleRow(row, key); \
    } \
    \
    static bool getMultiRow(std::vector<ClassName>& row_list, const std::vector<std::string>& key_list) \
    { \
        return ::sf1r::getMultiRow(row_list, key_list); \
    } \
    \
    /* static bool getRangeRow(std::vector<ClassName>& row_list, const std::string& start_key, const std::string& end_key); */ \


}

#endif
