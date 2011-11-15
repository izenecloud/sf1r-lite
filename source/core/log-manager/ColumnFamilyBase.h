#ifndef _COLUMN_FAMILY_BASE_H_
#define _COLUMN_FAMILY_BASE_H_

#include "CassandraConnection.h"

namespace sf1r {

class ColumnFamilyBase
{
public:
    ColumnFamilyBase() {}

    virtual ~ColumnFamilyBase() {}

    enum ColumnType {
        UNKNOWN,
        NORMAL,
        COUNTER
    };

    virtual bool truncateColumnFamily() const;

    virtual bool dropColumnFamily() const;

    virtual const std::string& getKey() const = 0;

    virtual const std::string& getName() const = 0;

    virtual const ColumnType getColumnType() const = 0;

    virtual bool updateRow() const = 0;

    virtual bool getSlice(const std::string& start, const std::string& finish);

    virtual bool deleteRow();

    virtual bool getCount(int32_t& count, const std::string& start, const std::string& finish) const;

    virtual bool insert(const std::string& name, const std::string& value) { return true; }

    virtual void insertCounter(const std::string& name, int64_t value) {}

    virtual void resetKey(const std::string& newKey = "") = 0;

    virtual void clear() = 0;
};

template <typename ColumnFamilyType>
const ColumnFamilyBase::ColumnType getColumnType()
{
    return ColumnFamilyType::column_type;
}

template <typename ColumnFamilyType>
const std::string& getName()
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
bool getSingleSlice(ColumnFamilyType& row, const std::string& key, const std::string& start, const std::string& finish)
{
    row.resetKey(key);
    return row.getSlice(start, finish);
}

template <typename ColumnFamilyType>
bool getSingleCount(int32_t& count, const std::string& key, const std::string& start, const std::string& finish)
{
    ColumnFamilyType row(key);
    return row.getCount(count, start, finish);
}

#define DEFINE_COLUMN_FAMILY_COMMON_ROUTINES(ClassName) \
    static const ColumnType column_type; \
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
    virtual const ColumnType getColumnType() const \
    { \
        return ::sf1r::getColumnType<ClassName>(); \
    } \
    \
    virtual const std::string& getName() const \
    { \
        return ::sf1r::getName<ClassName>(); \
    } \
    \
    static bool createColumnFamily() \
    { \
        return ::sf1r::createColumnFamily<ClassName>(); \
    } \
    \
    static bool getSingleSlice(ClassName& row, const std::string& key, const std::string& start, const std::string& finish) \
    { \
        return ::sf1r::getSingleSlice(row, key, start, finish); \
    } \
    \
    static bool getSingleCount(int32_t& count, const std::string& key, const std::string& start, const std::string& finish) \
    { \
        return ::sf1r::getSingleCount<ClassName>(count, key, start, finish); \
    } \
    \
    /* XXX Don't forget to implement the following two in your ColumnFamily class */ \
    static bool getMultiSlice( \
            std::map<std::string, ClassName>& row_map, \
            const std::vector<std::string>& key_list, \
            const std::string& start, \
            const std::string& finish); \
    \
    static bool getMultiCount( \
            std::map<std::string, int32_t>& count_map, \
            const std::vector<std::string>& key_list, \
            const std::string& start, \
            const std::string& finish); \


}

#endif
