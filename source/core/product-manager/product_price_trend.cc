#include "product_price_trend.h"
#include "product_manager.h"

#include <log-manager/PriceHistory.h>

#include <libcassandra/util_functions.h>

using namespace libcassandra;
using namespace boost::posix_time;
using izenelib::util::UString;

namespace sf1r
{

ProductPriceTrend::ProductPriceTrend(const std::string& collection_name, ProductManager* product_manager)
    : collection_name_(collection_name)
    , product_manager_(product_manager)
{
}

ProductPriceTrend::~ProductPriceTrend()
{
}

void ProductPriceTrend::Init()
{
    //TODO load some initial price data
}

bool ProductPriceTrend::Finish()
{
    Save_();
    return Flush_();
}

bool ProductPriceTrend::Insert(const PMDocumentType& doc, time_t timestamp, bool r_type)
{
    ProductPrice price;
    if (!product_manager_->GetPrice(doc, price)) return true;
    UString docid;
    if (!product_manager_->GetDOCID(doc, docid)) return true;
    std::string docid_str, key_str;
    docid.convertString(docid_str, UString::UTF_8);
    ParseDocid_(key_str, docid_str);
    product_manager_->GetTimestamp(doc, timestamp);

    price_history_cache_.push_back(PriceHistory(key_str));
    price_history_cache_.back().insert(timestamp, price);

    if (price_history_cache_.size() == 10000)
    {
        return Flush_();
    }
    return true;
}

bool ProductPriceTrend::Flush_()
{
    bool ret = PriceHistory::updateMultiRow(price_history_cache_);
    price_history_cache_.clear();
    return ret;
}

void ProductPriceTrend::Save_()
{
    //TODO save some price data to disk file
}

bool ProductPriceTrend::GetMultiPriceHistory(
        PriceHistoryList& history_list,
        const std::vector<std::string>& docid_list,
        time_t from_tt,
        time_t to_tt,
        std::string& error_msg)
{
    std::vector<std::string> key_list;
    ParseDocidList_(key_list, docid_list);

    std::string from_str(from_tt == -1 ? "" : serializeLong(from_tt));
    std::string to_str(to_tt == -1 ? "" : serializeLong(to_tt));

    std::map<std::string, PriceHistory> row_map;
    if (!PriceHistory::getMultiSlice(row_map, key_list, from_str, to_str))
    {
        error_msg = "Failed retrieving price histories from Cassandra";
        return false;
    }

    for (std::map<std::string, PriceHistory>::const_iterator it = row_map.begin();
            it != row_map.end(); ++it)
    {
        PriceHistoryItem history_item;
        for (PriceHistory::PriceHistoryType::const_iterator hit = it->second.getPriceHistory().begin();
                hit != it->second.getPriceHistory().end(); ++hit)
        {
            history_item.push_back(make_pair(std::string(), hit->second.value));
            history_item.back().first.assign(to_iso_string(
                        from_time_t(hit->first / 1000000 - timezone)
                        + microseconds(hit->first % 1000000)));
        }
        if (!history_item.empty())
        {
            history_list.push_back(make_pair(std::string(), PriceHistoryItem()));
            StripDocid_(history_list.back().first, it->first);
            history_list.back().second.swap(history_item);
        }
    }
    if (history_list.empty())
    {
        error_msg = "Have not got any valid docid";
        return false;
    }

    return true;
}

bool ProductPriceTrend::GetMultiPriceRange(
        PriceRangeList& range_list,
        const std::vector<std::string>& docid_list,
        time_t from_tt,
        time_t to_tt,
        std::string& error_msg)
{
    std::vector<std::string> key_list;
    ParseDocidList_(key_list, docid_list);

    std::string from_str(from_tt == -1 ? "" : serializeLong(from_tt));
    std::string to_str(to_tt == -1 ? "" : serializeLong(to_tt));

    std::map<std::string, PriceHistory> row_map;
    if (!PriceHistory::getMultiSlice(row_map, key_list, from_str, to_str))
    {
        error_msg = "Failed retrieving price histories from Cassandra";
        return false;
    }

    for (std::map<std::string, PriceHistory>::const_iterator it = row_map.begin();
            it != row_map.end(); ++it)
    {
        ProductPrice range_item;
        for (PriceHistory::PriceHistoryType::const_iterator hit = it->second.getPriceHistory().begin();
                hit != it->second.getPriceHistory().end(); ++hit)
        {
            range_item += hit->second;
        }
        if (range_item.Valid())
        {
            range_list.push_back(make_pair(std::string(), range_item.value));
            StripDocid_(range_list.back().first, it->first);
        }
    }
    if (range_list.empty())
    {
        error_msg = "Have not got any valid docid";
        return false;
    }

    return true;
}

void ProductPriceTrend::ParseDocid_(std::string& dest, const std::string& src) const
{
    if (!collection_name_.empty())
        dest.assign(collection_name_ + "_" + src);
    else
        dest.assign(src);
}

void ProductPriceTrend::StripDocid_(std::string& dest, const std::string& src) const
{
    if (!collection_name_.empty() && src.length() > collection_name_.length() + 1)
        dest.assign(src.substr(collection_name_.length() + 1));
    else
        dest.assign(src);
}

void ProductPriceTrend::ParseDocidList_(std::vector<std::string>& dest, const std::vector<std::string>& src) const
{
    for (uint32_t i = 0; i < src.size(); i++)
    {
        if (src[i].empty()) continue;
        dest.push_back(std::string());
        ParseDocid_(dest.back(), src[i]);
    }
}

void ProductPriceTrend::StripDocidList_(std::vector<std::string>& dest, const std::vector<std::string>& src) const
{
    for (uint32_t i = 0; i < src.size(); i++)
    {
        if (src[i].empty()) continue;
        dest.push_back(std::string());
        StripDocid_(dest.back(), src[i]);
    }
}

}
