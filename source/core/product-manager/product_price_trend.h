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
    // map<"docid", multimap<"price-cut", pair<"start time", "property value"> > >
    typedef std::map<std::string, std::multimap<float, std::pair<time_t, std::string> > > TopPriceCutMap;

public:
    ProductPriceTrend(const std::string& collection_name, const std::string& dir, const std::string& category_property, const std::string& source_property);

    ~ProductPriceTrend();

    void Init();

    bool Finish();

    bool Insert(const std::string& docid, const ProductPrice& price, time_t timestamp, const std::string& category = "", const std::string& source = "");

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

    void UpdateTPCMap_(TopPriceCutMap& tpc_map, const vector<PriceHistory>& row_list, const map<string, pair<ProductPriceType, string> >& cache_map);

    void ParseDocid_(std::string& dest, const std::string& src) const;

    void StripDocid_(std::string& dest, const std::string& src) const;

    void ParseDocidList_(std::vector<std::string>& dest, const std::vector<std::string>& src) const;

    void StripDocidList_(std::vector<std::string>& dest, const std::vector<std::string>& src) const;

    friend class boost::serialization::access;

    template <typename Archive>
    void serialize( Archive & ar, const unsigned int version )
    {
        ar & category_tpc_;
        ar & source_tpc_;
    }

private:
    std::string collection_name_;
    std::string top_price_cuts_file_;
    std::string category_property_;
    std::string source_property_;
    std::vector<PriceHistory> price_history_cache_;
    std::vector<TopPriceCutMap> category_tpc_;
    std::vector<TopPriceCutMap> source_tpc_;
    // map<"docid", pair<"low price", "property value"> >
    std::map<std::string, std::pair<ProductPriceType, std::string> > category_map_;
    std::map<std::string, std::pair<ProductPriceType, std::string> > source_map_;
};

}

#endif
