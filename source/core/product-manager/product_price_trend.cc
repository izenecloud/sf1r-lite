#include "product_price_trend.h"

#include <log-manager/PriceHistory.h>
#include <libcassandra/util_functions.h>

#include <algorithm>

using namespace std;
using namespace libcassandra;
using izenelib::util::UString;
using namespace boost;
using namespace boost::posix_time;

namespace sf1r
{

ProductPriceTrend::ProductPriceTrend(
        const string& collection_name,
        const string& dir,
        const string& category_property,
        const string& source_property)
    : collection_name_(collection_name)
    , category_property_(category_property)
    , source_property_(source_property)
{
    category_tpc_.push_back(new TPCStorage(dir + "/category_top_price_cut.0"));
    category_tpc_.push_back(new TPCStorage(dir + "/category_top_price_cut.1"));
    category_tpc_.push_back(new TPCStorage(dir + "/category_top_price_cut.2"));

    source_tpc_.push_back(new TPCStorage(dir + "/source_top_price_cut.0"));
    source_tpc_.push_back(new TPCStorage(dir + "/source_top_price_cut.1"));
    source_tpc_.push_back(new TPCStorage(dir + "/source_top_price_cut.2"));
}

ProductPriceTrend::~ProductPriceTrend()
{
    Flush();
    for (uint32_t i = 0; i < 3; i++)
    {
        delete category_tpc_[i];
        delete source_tpc_[i];
    }
}

bool ProductPriceTrend::Init()
{
    bool ret = true;

    for (uint32_t i = 0; i < 3; i++)
    {
        if (!category_tpc_[i]->open()) ret = false;
        if (!source_tpc_[i]->open()) ret = false;
    }

    return ret;
}

bool ProductPriceTrend::Insert(
        const string& docid,
        const ProductPrice& price,
        time_t timestamp,
        uint32_t num_docid,
        const string& category,
        const string& source)
{
    string key;
    ParseDocid_(key, docid);
    price_history_cache_.push_back(PriceHistory(key));
    price_history_cache_.back().insert(timestamp, price);

    if (price.value.first > 0 && num_docid > 0)
    {
        if (!category.empty())
        {
            category_map_[docid] = make_tuple(num_docid, price.value.first, category);
        }
        if (!source.empty())
        {
            source_map_[docid] = make_tuple(num_docid, price.value.first, source);
        }
    }

    if (price_history_cache_.size() == 10000)
    {
        return Flush();
    }
    return true;
}

bool ProductPriceTrend::CronJob()
{
    //TODO
    return true;
}

bool ProductPriceTrend::UpdateTPC_(TPCStorage* tpc_storage, const vector<string>& key_list, const CacheMap& cache_map, time_t timestamp)
{
    string from_str(serializeLong(timestamp));
    vector<PriceHistory> row_list;
    if (!PriceHistory::getMultiSlice(row_list, key_list, from_str, "", 1))
        return false;

    for (uint32_t i = 0; i < row_list.size(); i++)
    {
        string docid;
        StripDocid_(docid, row_list[i].getDocId());
        const CacheMap::mapped_type& cache_record = cache_map.find(docid)->second;

        const pair<time_t, ProductPrice>& price_record = *(row_list[i].getPriceHistory().begin());
        double old_price = price_record.second.value.first;
        float price_cut = old_price == 0 ? 0 : 1 - cache_record.get<1>() / old_price;

        TPCQueue tpc_queue;
        tpc_storage->get(cache_record.get<2>(), tpc_queue);
        tpc_queue.push_back(make_tuple(price_cut, price_record.first, cache_record.get<0>()));
        push_heap(tpc_queue.begin(), tpc_queue.end(), rel_ops::operator><TPCQueue::value_type>);

        if (tpc_queue.size() > 100)
        {
            pop_heap(tpc_queue.begin(), tpc_queue.end(), rel_ops::operator><TPCQueue::value_type>);
            tpc_queue.pop_back();
        }
        tpc_storage->update(cache_record.get<2>(), tpc_queue);
    }
    tpc_storage->flush();

    return true;
}

bool ProductPriceTrend::Flush()
{
    bool ret = true;
    time_t now = createTimeStamp();

    if (!category_map_.empty())
    {
        vector<string> docid_list, key_list;
        getKeyList(docid_list, category_map_);
        ParseDocidList_(key_list, docid_list);

        if (!UpdateTPC_(category_tpc_[0], key_list, category_map_, now - 86400000000L * 7))
            ret = false;

        if (!UpdateTPC_(category_tpc_[1], key_list, category_map_, now - 86400000000L * 183))
            ret = false;

        if (!UpdateTPC_(category_tpc_[2], key_list, category_map_, now - 86400000000L * 365))
            ret = false;

        category_map_.clear();
    }

    if (!source_map_.empty())
    {
        vector<string> docid_list, key_list;
        getKeyList(docid_list, source_map_);
        ParseDocidList_(key_list, docid_list);

        if (!UpdateTPC_(source_tpc_[0], key_list, source_map_, now - 86400000000L * 7))
            ret = false;

        if (!UpdateTPC_(source_tpc_[1], key_list, source_map_, now - 86400000000L * 183))
            ret = false;

        if (!UpdateTPC_(source_tpc_[2], key_list, source_map_, now - 86400000000L * 365))
            ret = false;

        source_map_.clear();
    }

    if (!PriceHistory::updateMultiRow(price_history_cache_))
        ret = false;

    price_history_cache_.clear();

    return ret;
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

bool ProductPriceTrend::GetTopPriceCutList(
        vector<pair<float, string> >& tpc_list,
        const string& prop_value,
        string& error_msg,
        ProductPriceTrend::PropertyName prop_name,
        ProductPriceTrend::TimeInterval time_int)
{
    TPCStorage* tpc_storage;
    switch (prop_name)
    {
    case CATEGORY:
        tpc_storage = category_tpc_[time_int];
        break;
    case SOURCE:
        tpc_storage = source_tpc_[time_int];
        break;
    default:
        return false;
    }

    TPCQueue tpc_queue;
    if (!tpc_storage->get(prop_value, tpc_queue))
    {
        error_msg = "Can't find this property name: " + prop_value;
        return false;
    }
    sort_heap(tpc_queue.begin(), tpc_queue.end(), rel_ops::operator><TPCQueue::value_type>);
    if (tpc_queue.size() > 20) tpc_queue.resize(20);
    sort(tpc_queue.begin(), tpc_queue.end(), tupleLess<2, TPCQueue::value_type>);
    //TODO convert the numeric doc ids to string-type ones

    return true;
}

}
