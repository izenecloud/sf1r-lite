#ifndef SF1R_PRODUCTMANAGER_PRODUCTPRICETREND_H
#define SF1R_PRODUCTMANAGER_PRODUCTPRICETREND_H

#include "pm_def.h"
#include "pm_types.h"
#include "product_price.h"

#include <am/tc/Hash.h>

#include <boost/tuple/tuple.hpp>

namespace sf1r
{

class PriceHistory;

class ProductPriceTrend
{
    typedef std::vector<boost::tuple<float, time_t, uint32_t> > TPCQueue;
    typedef izenelib::am::tc::Hash<std::string, TPCQueue> TPCStorage;
    typedef std::map<std::string, boost::tuple<uint32_t, ProductPriceType, std::string> > CacheMap;

public:
    ProductPriceTrend(
            const std::string& collection_name,
            const std::string& dir,
            const std::string& category_property,
            const std::string& source_property);

    ~ProductPriceTrend();

    bool Init();

    bool Flush();

    bool Insert(
            const std::string& docid,
            const ProductPrice& price,
            time_t timestamp,
            uint32_t num_docid = 0,
            const std::string& category = "",
            const std::string& source = "");

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

    enum TimeInterval
    {
        ONE_WEEK = 0,
        HALF_YEAR,
        ONE_YEAR
    };

    enum PropertyName
    {
        CATEGORY = 0,
        SOURCE
    };

    bool GetTopPriceCutList(
            std::vector<std::pair<float, std::string> >& tpc_list,
            const std::string& prop_value,
            std::string& error_msg,
            PropertyName prop_name = CATEGORY,
            TimeInterval time_int = ONE_WEEK);

    bool CronJob();

    inline const std::string& getCollectionName() const
    {
        return collection_name_;
    }

private:
    bool UpdateTPC_(TPCStorage* tpc_storage, const vector<string>& key_list, const CacheMap& cache_map, time_t timestamp);

    void ParseDocid_(std::string& dest, const std::string& src) const;

    void StripDocid_(std::string& dest, const std::string& src) const;

    void ParseDocidList_(std::vector<std::string>& dest, const std::vector<std::string>& src) const;

    void StripDocidList_(std::vector<std::string>& dest, const std::vector<std::string>& src) const;

private:
    std::string collection_name_;
    std::string category_property_;
    std::string source_property_;
    std::vector<PriceHistory> price_history_cache_;
    std::vector<TPCStorage *> category_tpc_;
    std::vector<TPCStorage *> source_tpc_;
    // map<"docid", pair<"low price", "property value"> >
    CacheMap category_map_;
    CacheMap source_map_;
};

}

#endif
