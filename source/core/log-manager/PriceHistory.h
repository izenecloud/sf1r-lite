#ifndef _PRICE_HISTORY_H_
#define _PRICE_HISTORY_H_

#include "CassandraConnection.h"

#include <product-manager/product_price.h>

namespace sf1r {

class PriceHistoryRow
{
public:
    typedef std::vector<std::pair<time_t, ProductPrice> > PriceHistoryType;

    explicit PriceHistoryRow(const uint128_t& docId = 0);

    ~PriceHistoryRow();

    bool insert(const std::string& name, const std::string& value);

    void resetKey(const uint128_t& newDocId = 0);

    void clear();

    void insert(time_t timestamp, ProductPrice price);

    void resetHistory(uint32_t index, time_t timestamp, ProductPrice price);

    inline const uint128_t& getDocId() const
    {
        return docId_;
    }

    inline void setDocId(const uint128_t& docId)
    {
        docId_ = docId;
    }

    inline const PriceHistoryType& getPriceHistory() const
    {
        return priceHistory_;
    }

    inline void setPriceHistory(const PriceHistoryType& priceHistory)
    {
        priceHistory_ = priceHistory;
        priceHistoryPresent_ = true;
    }

    inline bool hasPriceHistory() const
    {
        return priceHistoryPresent_;
    }

private:
    friend class PriceHistory;

    uint128_t docId_;

    PriceHistoryType priceHistory_;
    bool priceHistoryPresent_;
};

class PriceHistory
{
public:
    typedef std::vector<std::pair<time_t, ProductPrice> > PriceHistoryType;

    explicit PriceHistory(const std::string& keyspace_name);

    ~PriceHistory();

    void createColumnFamily();

    bool truncateColumnFamily() const;

    bool dropColumnFamily() const;

    bool updateRow(const PriceHistoryRow& row) const;

    bool getSlice(PriceHistoryRow& row, const std::string& start = "", const std::string& finish = "", int32_t count = 100, bool reversed = false);

    bool deleteRow(const std::string& key);

    bool getCount(int32_t& count, const std::string& key, const std::string& start = "", const std::string& finish = "") const;

    bool updateMultiRow(const std::vector<PriceHistoryRow>& row_list);

    bool getMultiSlice(
            std::vector<PriceHistoryRow>& row_list,
            const std::vector<uint128_t>& key_list,
            const std::string& start = "",
            const std::string& finish = "",
            int32_t count = 100,
            bool reversed = false);

    bool getMultiCount(
            std::vector<std::pair<uint128_t, int32_t> >& count_list,
            const std::vector<uint128_t>& key_list,
            const std::string& start = "",
            const std::string& finish = "");

public:
    bool is_enabled;
    /**
     * XXX Below are the metadata of the column family. Some of them cannot be
     * changed if the column family has already been created (and not yet
     * dropped) at server. So be cautious enough to determine them!
     */
    std::string keyspace_name_;
    static const std::string cf_name; /* XXX unchangeable */
    static const std::string cf_column_type; /* XXX unchangeable */
    static const std::string cf_comparator_type; /* XXX unchangeable */
    static const std::string cf_sub_comparator_type; /* XXX unchangeable */
    static const std::string cf_comment;
    static const double cf_row_cache_size;
    static const double cf_key_cache_size;
    static const double cf_read_repair_chance;
    static const std::vector<org::apache::cassandra::ColumnDef> cf_column_metadata;
    static const int32_t cf_gc_grace_seconds;
    static const std::string cf_default_validation_class;
    static const int32_t cf_id; /* XXX you never want to manually set this one */
    static const int32_t cf_min_compaction_threshold;
    static const int32_t cf_max_compaction_threshold;
    static const int32_t cf_row_cache_save_period_in_seconds;
    static const int32_t cf_key_cache_save_period_in_seconds;
    static const int8_t cf_replicate_on_write;
    static const double cf_merge_shards_chance;
    static const std::string cf_key_validation_class;
    static const std::string cf_row_cache_provider;
    static const std::string cf_key_alias;
    static const std::string cf_compaction_strategy;
    static const std::map<std::string, std::string> cf_compaction_strategy_options;
    static const int32_t cf_row_cache_keys_to_save;
    static const std::map<std::string, std::string> cf_compression_options;
    static const double cf_bloom_filter_fp_chance;

};

}
#endif /*_PRICE_HISTORY_H_ */
