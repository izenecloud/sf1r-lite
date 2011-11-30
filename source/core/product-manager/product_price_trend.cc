#include "product_price_trend.h"

#include <common/Utilities.h>
#include <log-manager/PriceHistory.h>
#include <am/range/AmIterator.h>
#include <libcassandra/util_functions.h>

#include <boost/lexical_cast.hpp>
#include <algorithm>

using namespace std;
using namespace libcassandra;
using izenelib::util::UString;

namespace sf1r
{

ProductPriceTrend::ProductPriceTrend(
        const string& collection_name,
        const string& data_dir,
        const vector<string>& group_props,
        const vector<uint32_t>& time_ints)
    : collection_name_(collection_name)
    , data_dir_(data_dir)
    , group_props_(group_props)
{
    set<uint32_t> ints_set;
    ints_set.insert(time_ints.begin(), time_ints.end());
    time_ints_.insert(time_ints_.begin(), ints_set.begin(), ints_set.end());
}

ProductPriceTrend::~ProductPriceTrend()
{
    Flush();
    for (TPCStorage::iterator it = tpc_storage_.begin();
            it != tpc_storage_.end(); ++it)
    {
        for (uint32_t i = 0; i < time_ints_.size(); i++)
            delete it->second[i];
    }
}

bool ProductPriceTrend::Init()
{
    if (!PriceHistory::is_enabled) return false;

    bool ret = true;

    for (vector<string>::const_iterator it = group_props_.begin();
            it != group_props_.end(); ++it)
    {
        vector<TPCBTree *>& prop_tpc = tpc_storage_[*it];
        for (uint32_t i = 0; i < time_ints_.size(); i++)
        {
            prop_tpc.push_back(new TPCBTree(data_dir_ + "/" + *it + "." + boost::lexical_cast<string>(time_ints_[i]) + ".tpc"));
            if (!prop_tpc.back()->open())
                ret = false;
//          else TraverseTPCBtree(*prop_tpc.back());
        }
    }

    return ret;
}

void ProductPriceTrend::TraverseTPCBtree(TPCBTree& tpc_btree)
{
    typedef izenelib::am::AMIterator<TPCBTree> AMIteratorType;
    AMIteratorType iter(tpc_btree);
    AMIteratorType end;
    for(; iter != end; ++iter)
    {
        cout << "====== Key: " << iter->first << endl;
        const TPCQueue& v = iter->second;

        cout << "====== Value:" << endl;
        for (TPCQueue::const_iterator vit = v.begin();
                vit != v.end(); ++vit)
        {
            cout << "\t" << vit->first << "\t" << vit->second << endl;
        }
    }
}

bool ProductPriceTrend::Insert(
        const string& docid,
        const ProductPrice& price,
        time_t timestamp)
{
    string key;
    ParseDocid_(key, docid);
    price_history_cache_.push_back(PriceHistory());
    price_history_cache_.back().resetKey(key);
    price_history_cache_.back().insert(timestamp, price);

    if (price_history_cache_.size() == 10000)
    {
        return Flush();
    }

    return true;
}

bool ProductPriceTrend::Update(
        const string& docid,
        const ProductPrice& price,
        time_t timestamp,
        map<string, string>& group_prop_map)
{
    string key;
    ParseDocid_(key, docid);
    price_history_cache_.push_back(PriceHistory());
    price_history_cache_.back().resetKey(key);
    price_history_cache_.back().insert(timestamp, price);

    if (!group_prop_map.empty() && price.value.first > 0)
    {
        PropCacheItem& cache_item = prop_cache_[key];
        cache_item.first = price.value.first;
        cache_item.second.swap(group_prop_map);
    }

    if (price_history_cache_.size() == 10000)
    {
        return Flush();
    }

    return true;
}

bool ProductPriceTrend::Flush()
{
    bool ret = true;
    time_t now = Utilities::createTimeStamp();

    if (!prop_cache_.empty())
    {
        for (uint32_t i = 0; i < time_ints_.size(); i++)
        {
            if (!UpdateTPC_(i, now))
                ret = false;
        }

        prop_cache_.clear();
    }

    if (!PriceHistory::updateMultiRow(price_history_cache_))
        ret = false;

    price_history_cache_.clear();

    return ret;
}

bool ProductPriceTrend::CronJob()
{
    //TODO
    return true;
}

bool ProductPriceTrend::UpdateTPC_(uint32_t time_int, time_t timestamp)
{
    vector<string> key_list;
    Utilities::getKeyList(key_list, prop_cache_);
    if (timestamp == -1)
        timestamp = Utilities::createTimeStamp();
    timestamp -= 86400000000L * time_ints_[time_int];

    vector<PriceHistory> row_list;
    if (!PriceHistory::getMultiSlice(row_list, key_list, serializeLong(timestamp), "", 1))
        return false;

    map<string, map<string, TPCQueue> > tpc_cache;
    for (uint32_t i = 0; i < row_list.size(); i++)
    {
        const PropCacheItem& cache_item = prop_cache_.find(row_list[i].getDocId())->second;
        const pair<time_t, ProductPrice>& price_record = *(row_list[i].getPriceHistory().begin());
        const ProductPriceType& old_price = price_record.second.value.first;
        float price_cut = old_price == 0 ? 0 : 1 - cache_item.first / old_price;

        for (map<string, string>::const_iterator it = cache_item.second.begin();
                it != cache_item.second.end(); ++it)
        {
            TPCQueue& tpc_queue = tpc_cache[it->first][it->second];
            if (tpc_queue.empty())
                tpc_storage_[it->first][time_int]->get(it->second, tpc_queue);

            tpc_queue.push_back(make_pair(price_cut, string()));
            StripDocid_(tpc_queue.back().second, row_list[i].getDocId());
            push_heap(tpc_queue.begin(), tpc_queue.end(), rel_ops::operator> <TPCQueue::value_type>);

            if (tpc_queue.size() > 1000)
            {
                pop_heap(tpc_queue.begin(), tpc_queue.end(), rel_ops::operator> <TPCQueue::value_type>);
                tpc_queue.pop_back();
            }
        }
    }

    for (map<string, map<string, TPCQueue> >::const_iterator it = tpc_cache.begin();
            it != tpc_cache.end(); ++it)
    {
        TPCBTree* tpc_btree = tpc_storage_[it->first][time_int];
        for (map<string, TPCQueue>::const_iterator mit = it->second.begin();
                mit != it->second.end(); ++mit)
        {
            tpc_btree->update(mit->first, mit->second);
        }
        tpc_btree->flush();
    }

    return true;
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
            history_item.back().first.assign(boost::posix_time::to_iso_string(
                        boost::posix_time::from_time_t(hit->first / 1000000 - timezone)
                        + boost::posix_time::microseconds(hit->first % 1000000)));
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

bool ProductPriceTrend::GetTopPriceCutList(
        TPCQueue& tpc_queue,
        const string& prop_name,
        const string& prop_value,
        uint32_t days,
        string& error_msg)
{
    TPCStorage::const_iterator tpc_it = tpc_storage_.find(prop_name);
    if (tpc_it == tpc_storage_.end())
    {
        error_msg = "Don't find data for this property name: " + prop_name;
        return false;
    }

    uint32_t time_int = 0;
    while (days > time_ints_[time_int++] && time_int < time_ints_.size() - 1);
    if (!tpc_it->second[time_int]->get(prop_value, tpc_queue))
    {
        error_msg = "Don't find price-cut record for this property value: " + prop_value;
        return false;
    }

    sort_heap(tpc_queue.begin(), tpc_queue.end(), rel_ops::operator><TPCQueue::value_type>);
    if (tpc_queue.size() > 20)
        tpc_queue.resize(20);

    return true;
}

}
