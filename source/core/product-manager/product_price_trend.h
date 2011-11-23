#ifndef SF1R_PRODUCTMANAGER_PRODUCTPRICETREND_H
#define SF1R_PRODUCTMANAGER_PRODUCTPRICETREND_H

#include "pm_def.h"
#include "pm_types.h"
#include "product_price.h"

namespace sf1r
{

class PriceHistory;

class ProductPriceTrend
{
    typedef std::map<uint32_t, std::vector<std::multimap<float, uint32_t> > > TopPriceCutMap;

public:
    ProductPriceTrend(const std::string& collection_name, const std::string& dir, const std::string& category_property, const std::string& source_property);

    ~ProductPriceTrend();

    void Init();

    bool Finish();

    bool Insert(const std::string& docid, const ProductPrice& price, time_t timestamp);

    bool Update(uint32_t category_id, uint32_t source_id, uint32_t num_docid, const std::string& str_docid, const ProductPrice& from_price, const ProductPrice& to_price, time_t timestamp);

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

    bool CronJob();

    inline const std::string& getCollectionName() const
    {
        return collection_name_;
    }

private:
    bool Flush_();

    void Save_();

    bool GetPriceCut_(std::vector<float>& price_cuts, uint32_t docid) const;

    bool SavePriceCut_(const std::vector<float>& price_cuts, uint32_t docid) const;

    bool UpdateTopPriceCuts_(TopPriceCutMap& top_map, uint32_t map_id, float price_cut, time_t timestamp, uint32_t docid);

    void ParseDocid_(std::string& dest, const std::string& src) const;

    void StripDocid_(std::string& dest, const std::string& src) const;

    void ParseDocidList_(std::vector<std::string>& dest, const std::vector<std::string>& src) const;

    void StripDocidList_(std::vector<std::string>& dest, const std::vector<std::string>& src) const;

private:
    std::string collection_name_;
    std::string dir_;
    std::string category_property_;
    std::string source_property_;
    std::vector<PriceHistory> price_history_cache_;
    TopPriceCutMap category_top_map_;
    TopPriceCutMap source_top_map_;
};

}

#endif
