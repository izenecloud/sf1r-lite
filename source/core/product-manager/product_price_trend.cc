#include "product_price_trend.h"

#include <log-manager/PriceHistory.h>
#include <libcassandra/util_functions.h>

#include <algorithm>

using namespace std;
using namespace libcassandra;
using namespace boost::posix_time;
using izenelib::util::UString;

namespace sf1r
{

ProductPriceTrend::ProductPriceTrend(const string& collection_name, const string& dir, const string& category_property, const string& source_property)
    : collection_name_(collection_name)
    , top_price_cuts_file_(dir + "/top_price_cuts.data")
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
    ifstream ifs(top_price_cuts_file_.c_str());
    if (!ifs.fail())
    {
        boost::archive::binary_iarchive ia(ifs);
        ia >> *this;
    }

    if (category_tpc_.empty())
    {
        category_tpc_.resize(3);
    }
    if (source_tpc_.empty())
    {
        source_tpc_.resize(3);
    }
}

bool ProductPriceTrend::Finish()
{
    Save_();
    return Flush_();
}

bool ProductPriceTrend::Insert(const string& docid, const ProductPrice& price, time_t timestamp, const string& category, const string& source)
{
    string key;
    ParseDocid_(key, docid);
    price_history_cache_.push_back(PriceHistory(key));
    price_history_cache_.back().insert(timestamp, price);

    if (!category.empty() && price.value.first > 0)
    {
        category_map_[docid] = make_pair(price.value.first, category);
    }
    if (!source.empty() && price.value.first > 0)
    {
        source_map_[docid] = make_pair(price.value.first, source);
    }

    if (price_history_cache_.size() == 10000)
    {
        return Flush_();
    }
    return true;
}

bool ProductPriceTrend::CronJob()
{
    //TODO
    return true;
}

void ProductPriceTrend::UpdateTPCMap_(TopPriceCutMap& tpc_map, const vector<PriceHistory>& row_list, const map<string, pair<ProductPriceType, string> >& cache_map)
{
    uint32_t i = 0;

    while (tpc_map.size() < 50 && i < row_list.size())
    {
        string docid;
        StripDocid_(docid, row_list[i].getDocId());
        const pair<time_t, ProductPrice>& price_record = *(row_list[i].getPriceHistory().begin());
        const pair<ProductPriceType, string>& cache_record = cache_map.find(docid)->second;

        float price_cut;
        double old_price = price_record.second.value.first;
        if (old_price == 0)
        {
            price_cut = 1;
        }
        else
        {
            price_cut = cache_record.first / old_price;
        }

        tpc_map[cache_record.second].insert(make_pair(price_cut, make_pair(price_record.first, docid)));
        i++;
    }

    for (; i < row_list.size(); i++)
    {
        string docid;
        StripDocid_(docid, row_list[i].getDocId());
        const pair<time_t, ProductPrice>& price_record = *(row_list[i].getPriceHistory().begin());
        const pair<ProductPriceType, string>& cache_record = cache_map.find(docid)->second;

        float price_cut;
        double old_price = price_record.second.value.first;
        if (old_price == 0)
        {
            price_cut = 1;
        }
        else
        {
            price_cut = cache_record.first / old_price;
        }

        tpc_map[cache_record.second].insert(make_pair(price_cut, make_pair(price_record.first, docid)));

        TopPriceCutMap::mapped_type::iterator it(tpc_map[cache_record.second].end());
        tpc_map[cache_record.second].erase(--it);
    }
}

bool ProductPriceTrend::Flush_()
{
    bool ret = true;
    time_t now = createTimeStamp();

    if (!category_map_.empty())
    {
        vector<string> docid_list, key_list;
        getKeyList(docid_list, category_map_);
        ParseDocidList_(key_list, docid_list);

        string from_str(serializeLong(now - 365 * 86400000000L));

        vector<PriceHistory> row_list;
        if (PriceHistory::getMultiSlice(row_list, key_list, from_str, "", 1))
        {
            UpdateTPCMap_(category_tpc_[0], row_list, category_map_);
        }
        else
        {
            ret = false;
        }

        from_str.assign(serializeLong(now - 183 * 86400000000L));
        row_list.clear();
        if (PriceHistory::getMultiSlice(row_list, key_list, from_str, "", 1))
        {
            UpdateTPCMap_(category_tpc_[0], row_list, category_map_);
        }
        else
        {
            ret = false;
        }

        from_str.assign(serializeLong(now - 7 * 86400000000L));
        row_list.clear();
        if (PriceHistory::getMultiSlice(row_list, key_list, from_str, "", 1))
        {
            UpdateTPCMap_(category_tpc_[0], row_list, category_map_);
        }
        else
        {
            ret = false;
        }

        category_map_.clear();
    }

    if (!source_map_.empty())
    {
        vector<string> docid_list, key_list;
        getKeyList(docid_list, source_map_);
        ParseDocidList_(key_list, docid_list);

        string from_str(serializeLong(now - 365 * 86400000000L));

        vector<PriceHistory> row_list;
        if (PriceHistory::getMultiSlice(row_list, key_list, from_str, "", 1))
        {
            UpdateTPCMap_(source_tpc_[0], row_list, source_map_);
        }
        else
        {
            ret = false;
        }

        from_str.assign(serializeLong(now - 183 * 86400000000L));
        row_list.clear();
        if (PriceHistory::getMultiSlice(row_list, key_list, from_str, "", 1))
        {
            UpdateTPCMap_(source_tpc_[0], row_list, source_map_);
        }
        else
        {
            ret = false;
        }

        from_str.assign(serializeLong(now - 7 * 86400000000L));
        row_list.clear();
        if (PriceHistory::getMultiSlice(row_list, key_list, from_str, "", 1))
        {
            UpdateTPCMap_(source_tpc_[0], row_list, source_map_);
        }
        else
        {
            ret = false;
        }

        source_map_.clear();
    }

    if (!PriceHistory::updateMultiRow(price_history_cache_))
    {
        ret = false;
    }
    price_history_cache_.clear();

    return ret;
}

void ProductPriceTrend::Save_()
{
    ofstream ofs(top_price_cuts_file_.c_str());
    boost::archive::binary_oarchive oa(ofs);
    oa << *this;
}

bool ProductPriceTrend::GetMultiPriceHistory(
        PriceHistoryList& history_list,
        const vector<string>& docid_list,
        time_t from_tt,
        time_t to_tt,
        string& error_msg)
{
    vector<string> key_list;
    ParseDocidList_(key_list, docid_list);

    string from_str(from_tt == -1 ? "" : serializeLong(from_tt));
    string to_str(to_tt == -1 ? "" : serializeLong(to_tt));

    vector<PriceHistory> row_list;
    if (!PriceHistory::getMultiSlice(row_list, key_list, from_str, to_str))
    {
        error_msg = "Failed retrieving price histories from Cassandra";
        return false;
    }

    for (vector<PriceHistory>::const_iterator it = row_list.begin();
            it != row_list.end(); ++it)
    {
        if (it->getPriceHistory().empty()) continue;
        PriceHistoryItem history_item;
        for (PriceHistory::PriceHistoryType::const_iterator hit = it->getPriceHistory().begin();
                hit != it->getPriceHistory().end(); ++hit)
        {
            history_item.push_back(make_pair(string(), hit->second.value));
            history_item.back().first.assign(to_iso_string(
                        from_time_t(hit->first / 1000000 - timezone)
                        + microseconds(hit->first % 1000000)));
        }
        history_list.push_back(make_pair(string(), PriceHistoryItem()));
        StripDocid_(history_list.back().first, it->getDocId());
        history_list.back().second.swap(history_item);
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
        const vector<string>& docid_list,
        time_t from_tt,
        time_t to_tt,
        string& error_msg)
{
    vector<string> key_list;
    ParseDocidList_(key_list, docid_list);

    string from_str(from_tt == -1 ? "" : serializeLong(from_tt));
    string to_str(to_tt == -1 ? "" : serializeLong(to_tt));

    vector<PriceHistory> row_list;
    if (!PriceHistory::getMultiSlice(row_list, key_list, from_str, to_str))
    {
        error_msg = "Failed retrieving price histories from Cassandra";
        return false;
    }

    for (vector<PriceHistory>::const_iterator it = row_list.begin();
            it != row_list.end(); ++it)
    {
        if (it->getPriceHistory().empty()) continue;
        ProductPrice range_item;
        for (PriceHistory::PriceHistoryType::const_iterator hit = it->getPriceHistory().begin();
                hit != it->getPriceHistory().end(); ++hit)
        {
            range_item += hit->second;
        }
        range_list.push_back(make_pair(string(), range_item.value));
        StripDocid_(range_list.back().first, it->getDocId());
    }

    if (range_list.empty())
    {
        error_msg = "Have not got any valid docid";
        return false;
    }

    return true;
}

void ProductPriceTrend::ParseDocid_(string& dest, const string& src) const
{
    if (!collection_name_.empty())
    {
        dest.assign(collection_name_ + "_" + src);
    }
    else
    {
        dest.assign(src);
    }
}

void ProductPriceTrend::StripDocid_(string& dest, const string& src) const
{
    if (!collection_name_.empty() && src.length() > collection_name_.length() + 1)
    {
        dest.assign(src.substr(collection_name_.length() + 1));
    }
    else
    {
        dest.assign(src);
    }
}

void ProductPriceTrend::ParseDocidList_(vector<string>& dest, const vector<string>& src) const
{
    for (uint32_t i = 0; i < src.size(); i++)
    {
        if (src[i].empty()) continue;
        dest.push_back(string());
        ParseDocid_(dest.back(), src[i]);
    }
}

void ProductPriceTrend::StripDocidList_(vector<string>& dest, const vector<string>& src) const
{
    for (uint32_t i = 0; i < src.size(); i++)
    {
        if (src[i].empty()) continue;
        dest.push_back(string());
        StripDocid_(dest.back(), src[i]);
    }
}

}
