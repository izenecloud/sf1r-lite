#include "product_price_trend.h"

#include <log-manager/PriceHistory.h>
#include <libcassandra/util_functions.h>

#include <algorithm>

using namespace libcassandra;
using namespace boost::posix_time;
using izenelib::util::UString;

namespace sf1r
{

ProductPriceTrend::ProductPriceTrend(const std::string& collection_name, const std::string& dir, const std::string& category_property, const std::string& source_property)
    : collection_name_(collection_name)
    , top_price_cuts_file_(dir + "/top_price_cuts.data")
    , price_history_file_(dir + "/price_history.data")
    , category_property_(category_property)
    , source_property_(source_property)
{
}

ProductPriceTrend::~ProductPriceTrend()
{
    Finish();
}

void ProductPriceTrend::Init()
{
    std::ifstream ifs(top_price_cuts_file_.c_str());
    if (!ifs.eof())
    {
        boost::archive::binary_iarchive ia(ifs);
        ia >> *this;
        ifs.close();
    }
}

bool ProductPriceTrend::Finish()
{
    Save_();
    return Flush_();
}

bool ProductPriceTrend::Insert(const std::string& docid, const ProductPrice& price, time_t timestamp)
{
    std::string key;
    ParseDocid_(key, docid);
    price_history_cache_.push_back(PriceHistory(key));
    price_history_cache_.back().insert(timestamp, price);

    if (price_history_cache_.size() == 10000)
    {
        return Flush_();
    }
    return true;
}

bool ProductPriceTrend::Update(const std::string& category, const std::string& source, uint32_t num_docid, const std::string& str_docid, const ProductPrice& from_price, const ProductPrice& to_price, time_t timestamp)
{
    bool ret = true;

    std::string key;
    ParseDocid_(key, str_docid);
    price_history_cache_.push_back(PriceHistory(key));
    price_history_cache_.back().insert(timestamp, to_price);

    float price_cut = from_price.value.first > 0 ? to_price.value.first / from_price.value.first : 1;
    if (!category.empty())
    {
        ret = UpdateTopPriceCuts_(category_top_map_[category], price_cut, timestamp, num_docid) && ret;
    }
    if (!source.empty())
    {
        ret = UpdateTopPriceCuts_(source_top_map_[source], price_cut, timestamp, num_docid) && ret;
    }

    if (price_history_cache_.size() == 10000)
    {
        ret = Flush_() && ret;
    }

    return ret;
}

bool ProductPriceTrend::CronJob()
{
    //TODO
    return true;
}

bool ProductPriceTrend::UpdateTopPriceCuts_(TopPriceCutMap::mapped_type& top_price_cuts, float price_cut, time_t timestamp, uint32_t docid)
{
    std::vector<float> price_cuts(3);
    if (!GetPriceCut_(price_cuts, docid)) return false;
    if (top_price_cuts.empty())
    {
        top_price_cuts.resize(3);
    }
    for (uint32_t i = 0; i < 3; i++)
    {
        price_cuts[i] = price_cuts[i] == 0 ? 1 : price_cuts[i] * price_cut;

        top_price_cuts[i].insert(std::make_pair(price_cuts[i], docid));
        if (top_price_cuts[i].size() > 20)
        {
            std::multimap<float, uint32_t>::iterator it(top_price_cuts[i].end());
            top_price_cuts[i].erase(--it);
        }
    }
    return SavePriceCut_(price_cuts, docid);
}

bool ProductPriceTrend::GetPriceCut_(std::vector<float>& price_cuts, uint32_t docid) const
{
    //TODO
    return true;
}

bool ProductPriceTrend::SavePriceCut_(const std::vector<float>& price_cuts, uint32_t docid) const
{
    //TODO
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
    std::ofstream ofs(top_price_cuts_file_.c_str());
    boost::archive::binary_oarchive oa(ofs);
    oa << *this;
    ofs.close();
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
