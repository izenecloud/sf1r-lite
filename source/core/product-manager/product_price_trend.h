#ifndef SF1R_PRODUCTMANAGER_PRODUCTPRICETREND_H
#define SF1R_PRODUCTMANAGER_PRODUCTPRICETREND_H

#include "pm_def.h"
#include "pm_types.h"
#include "pm_config.h"
#include "product_price.h"

namespace sf1r
{

class ProductManager;
class PriceHistory;

class ProductPriceTrend
{
public:
    ProductPriceTrend(const std::string& collection_name, ProductManager* product_manager);

    ~ProductPriceTrend();

    void Init();

    bool Finish();

    bool Insert(const PMDocumentType& doc, time_t timestamp, bool r_type = false);

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

private:
    bool Flush_();

    void Save_();

    void ParseDocid_(std::string& dest, const std::string& src) const;

    void StripDocid_(std::string& dest, const std::string& src) const;

    void ParseDocidList_(std::vector<std::string>& dest, const std::vector<std::string>& src) const;

    void StripDocidList_(std::vector<std::string>& dest, const std::vector<std::string>& src) const;

private:
    std::string collection_name_;
    ProductManager* product_manager_;
    std::vector<PriceHistory> price_history_cache_;
    std::vector<std::vector<std::pair<uint32_t, ProductPriceType> > > category_tops_;
};

}

#endif
