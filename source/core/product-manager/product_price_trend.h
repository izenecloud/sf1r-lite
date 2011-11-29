#ifndef SF1R_PRODUCTMANAGER_PRODUCTPRICETREND_H
#define SF1R_PRODUCTMANAGER_PRODUCTPRICETREND_H

#include "pm_def.h"
#include "pm_types.h"
#include "product_price.h"

#include <am/tc/BTree.h>

namespace sf1r
{

class PriceHistory;

class ProductPriceTrend
{
    typedef izenelib::am::tc::BTree<std::string, TPCQueue> TPCBTree;
    typedef std::map<std::string, std::vector<TPCBTree *> > TPCStorage;

    typedef std::pair<ProductPriceType, std::map<std::string, std::string> > PropCacheItem;
    typedef std::map<std::string, PropCacheItem> PropCacheMap;

public:
    ProductPriceTrend(
            const std::string& collection_name,
            const std::string& data_dir,
            const std::vector<std::string>& group_props,
            const std::vector<uint32_t>& time_ints);

    ~ProductPriceTrend();

    bool Init();

    bool Flush();

    bool Insert(
            const std::string& docid,
            const ProductPrice& price,
            time_t timestamp);

    bool Update(
            const std::string& docid,
            const ProductPrice& price,
            time_t timestamp,
            std::map<std::string, std::string>& group_prop_map);

    bool GetMultiPriceHistory(
            PriceHistoryList& history_list,
            const std::vector<std::string>& docid_list,
            time_t from_tt,
            time_t to_tt,
            std::string& error_msg);

    bool GetMultiPriceRange(
            PriceRangeList& range_list,
            const std::vector<std::string>& docid_list,
            time_t from_tt,
            time_t to_tt,
            std::string& error_msg);

    bool GetTopPriceCutList(
            TPCQueue& tpc_queue,
            const std::string& prop_name,
            const std::string& prop_value,
            uint32_t days,
            std::string& error_msg);

    bool CronJob();

    void TraverseTPCBtree(TPCBTree& tpc_btree);

    inline const std::string& getCollectionName() const
    {
        return collection_name_;
    }

private:
    bool UpdateTPC_(uint32_t time_int, time_t timestamp = -1);

    void ParseDocid_(std::string& dest, const std::string& src) const;

    void StripDocid_(std::string& dest, const std::string& src) const;

    void ParseDocidList_(std::vector<std::string>& dest, const std::vector<std::string>& src) const;

    void StripDocidList_(std::vector<std::string>& dest, const std::vector<std::string>& src) const;

private:
    std::string collection_name_;
    std::string data_dir_;
    std::vector<std::string> group_props_;
    std::vector<uint32_t> time_ints_;
    std::vector<PriceHistory> price_history_cache_;
    PropCacheMap prop_cache_;
    TPCStorage tpc_storage_;
};

}

#endif
