#include "product_manager.h"
#include "product_data_source.h"
#include "uuid_generator.h"
#include "product_price_trend.h"
#include "pm_util.h"

#include <log-manager/LogServerRequest.h>
#include <log-manager/LogServerConnection.h>
#include <document-manager/DocumentManager.h>
#include <node-manager/DistributeRequestHooker.h>

#include <common/ScdWriter.h>
#include <common/Utilities.h>

#include <boost/unordered_set.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/dynamic_bitset.hpp>
#include <glog/logging.h>

namespace sf1r
{

using izenelib::util::UString;

//#define PM_PROFILER

ProductManager::ProductManager(
        const std::string& work_dir,
        const boost::shared_ptr<DocumentManager>& document_manager,
        ProductDataSource* data_source,
        ProductPriceTrend* price_trend,
        const PMConfig& config)
    : work_dir_(work_dir)
    , config_(config)
    , document_manager_(document_manager)
    , data_source_(data_source)
    , price_trend_(price_trend)
    , util_(new PMUtil(config, data_source))
    , has_price_trend_(false)
    , inhook_(false)
{
    if (price_trend_)
    {
        if (price_trend_->Init())
            has_price_trend_ = true;
        else
            std::cerr << "Error: Price trend has not been properly initialized" << std::endl;
    }
    ///UpdateTPC_ in ProductPriceTrend::Update might be pretty slow due to 
    ///the performance of Cassandra. Without setting the capacity of jobScheduler,
    ///the data within jobScheduler will be accumulated so that the memory will be
    ///occupied a lot
    jobScheduler_.setCapacity(100000);
//     if (!config_.backup_path.empty())
//     {
//         backup_ = new ProductBackup(config_.backup_path);
//     }
}

ProductManager::~ProductManager()
{
    delete util_;
}

bool ProductManager::HookInsert(const PMDocumentType& doc, time_t timestamp)
{
    inhook_ = true;
    boost::mutex::scoped_lock lock(human_mutex_);

    if (has_price_trend_)
    {
        ProductPrice price;
        if (util_->GetPrice(doc, price))
        {
            UString docid;
            if (doc.getProperty(config_.docid_property_name, docid))
            {
                std::string docid_str;
                docid.convertString(docid_str, UString::UTF_8);
                if (timestamp == -1) GetTimestamp_(doc, timestamp);
                uint128_t num_docid = Utilities::md5ToUint128(docid_str);
                task_type task = boost::bind(&ProductPriceTrend::Insert, price_trend_, num_docid, price, timestamp);
                jobScheduler_.addTask(task);
            }
        }
    }
    return true;
}

bool ProductManager::HookUpdate(const PMDocumentType& to, docid_t oldid, time_t timestamp)
{
    inhook_ = true;
    boost::mutex::scoped_lock lock(human_mutex_);

    uint32_t fromid = oldid; //oldid
    PMDocumentType from;
    if (!data_source_->GetDocument(fromid, from,true))
    {
        inhook_ = false;
        return false;
    }
    UString from_uuid;
    if (!from.getProperty(config_.uuid_property_name, from_uuid) )
    {
        inhook_ = false;
        return false;
    }
    ProductPrice from_price;
    ProductPrice to_price;
    util_->GetPrice(fromid, from_price);
    util_->GetPrice(to, to_price);

    if (has_price_trend_ && to_price.Valid() && from_price != to_price)
    {
        UString docid;
        if (to.getProperty(config_.docid_property_name, docid ) )
        {
            std::string docid_str;
            docid.convertString(docid_str, UString::UTF_8);
            if (timestamp == -1) GetTimestamp_(to, timestamp);
            uint128_t num_docid = Utilities::md5ToUint128(docid_str);
            std::map<std::string, std::string> group_prop_map;
            GetGroupProperties_(to, group_prop_map);
            task_type task = boost::bind(&ProductPriceTrend::Update, price_trend_, num_docid, to_price, timestamp, group_prop_map);
            //task_type task = boost::bind(&ProductPriceTrend::Insert, price_trend_, num_docid, to_price, timestamp);
            jobScheduler_.addTask(task);
        }
    }

    return true;
}

bool ProductManager::HookDelete(uint32_t docid, time_t timestamp)
{
    inhook_ = true;
    boost::mutex::scoped_lock lock(human_mutex_);
    return true;
}

bool ProductManager::FinishHook(time_t timestamp)
{
    if (has_price_trend_)
    {
        task_type task = boost::bind(&ProductPriceTrend::Flush, price_trend_, timestamp);
        jobScheduler_.addTask(task);
    }
    if (inhook_) inhook_ = false;
    return true;
}

bool ProductManager::GetMultiPriceHistory(
        PriceHistoryList& history_list,
        const std::vector<uint128_t>& docid_list,
        time_t from_tt,
        time_t to_tt)
{
    if (!has_price_trend_)
    {
        error_ = "Price trend is not enabled for this collection";
        return false;
    }

    return price_trend_->GetMultiPriceHistory(history_list, docid_list, from_tt, to_tt, error_);
}

bool ProductManager::GetMultiPriceRange(
        PriceRangeList& range_list,
        const std::vector<uint128_t>& docid_list,
        time_t from_tt,
        time_t to_tt)
{
    if (!has_price_trend_)
    {
        error_ = "Price trend is not enabled for this collection";
        return false;
    }

    return price_trend_->GetMultiPriceRange(range_list, docid_list, from_tt, to_tt, error_);
}

bool ProductManager::GetTopPriceCutList(
        TPCQueue& tpc_queue,
        const std::string& prop_name,
        const std::string& prop_value,
        uint32_t days,
        uint32_t count)
{
    if (!has_price_trend_)
    {
        error_ = "Price trend is not enabled for this collection";
        return false;
    }

    return price_trend_->GetTopPriceCutList(tpc_queue, prop_name, prop_value, days, count, error_);
}

bool ProductManager::MigratePriceHistory(
        const std::string& new_keyspace,
        uint32_t start)
{
    if (!has_price_trend_)
    {
        error_ = "Price trend is not enabled for this collection";
        return false;
    }

    return price_trend_->MigratePriceHistory(document_manager_, new_keyspace, start, error_);
}

bool ProductManager::GetTimestamp_(const PMDocumentType& doc, time_t& timestamp) const
{
    boost::shared_ptr<NumericPropertyTableBase>& date_table
        = document_manager_->getNumericPropertyTable(config_.date_property_name);

    if (date_table && date_table->getInt64Value(doc.getId(), timestamp))
    {
        timestamp *= 1000000; // convert seconds to microseconds
        return true;
    }

    timestamp = Utilities::createTimeStamp();
    return false;
}

bool ProductManager::GetGroupProperties_(const PMDocumentType& doc, std::map<std::string, std::string>& group_prop_map) const
{
    for (uint32_t i = 0; i < config_.group_property_names.size(); i++)
    {
        Document::property_const_iterator it = doc.findProperty(config_.group_property_names[i]);
        if (it == doc.propertyEnd()) continue;
        UString prop_ustr = it->second.get<UString>();
        std::string& prop_str = group_prop_map[config_.group_property_names[i]];
        prop_ustr.convertString(prop_str, UString::UTF_8);
        size_t pos;
        if ((pos = prop_str.find('>')) != std::string::npos)
            prop_str.resize(pos);
    }
    return group_prop_map.empty();
}

}
