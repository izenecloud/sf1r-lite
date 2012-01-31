#include "product_manager.h"
#include "product_data_source.h"
#include "operation_processor.h"
#include "uuid_generator.h"
#include "product_editor.h"
#include "product_clustering.h"
#include "product_price_trend.h"

#include <common/ScdWriter.h>
#include <common/Utilities.h>

//#define USE_LOG_SERVER
#ifdef USE_LOG_SERVER
#include <log-manager/LogServerRequest.h>
#include <log-manager/LogServerConnection.h>
#endif

#include <boost/unordered_set.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/dynamic_bitset.hpp>
#include <glog/logging.h>

using namespace sf1r;
using izenelib::util::UString;

//#define PM_PROFILER

ProductManager::ProductManager(
        const std::string& work_dir,
        ProductDataSource* data_source,
        OperationProcessor* op_processor,
        ProductPriceTrend* price_trend,
        const PMConfig& config)
    : work_dir_(work_dir)
    , config_(config)
    , data_source_(data_source)
    , op_processor_(op_processor)
    , price_trend_(price_trend)
    , clustering_(NULL)
    , editor_(new ProductEditor(data_source, op_processor, config))
    , util_(config, data_source)
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

//     if (!config_.backup_path.empty())
//     {
//         backup_ = new ProductBackup(config_.backup_path);
//     }

#ifdef USE_LOG_SERVER
    LogServerConnection& conn = LogServerConnection::instance();
    conn.init("localhost", 18811); // TODO: make address configurable
#endif
}

ProductManager::~ProductManager()
{
    delete editor_;
}

bool ProductManager::Recover()
{
//     if (backup_==NULL)
//     {
//         error_ = "Backup not initialzed.";
//         return false;
//     }
//     if (!backup_->Recover(this))
//     {
// //         error_ = "Backup recover failed.";
//         return false;
//     }
//     if (!GenOperations_())
//     {
//         return false;
//     }
//     return true;
    return false;
}

ProductClustering* ProductManager::GetClustering_()
{
    if (!clustering_)
    {
        if (config_.enable_clustering_algo)
        {
            clustering_ = new ProductClustering(work_dir_+"/clustering", config_);
            if (!clustering_->Open())
            {
                std::cout<<"ProductClustering open failed"<<std::endl;
                delete clustering_;
                clustering_ = NULL;
            }
        }
    }
    return clustering_;
}

bool ProductManager::HookInsert(PMDocumentType& doc, izenelib::ir::indexmanager::IndexerDocument& index_document, time_t timestamp)
{
    inhook_ = true;
    boost::mutex::scoped_lock lock(human_mutex_);

    if (has_price_trend_)
    {
        ProductPrice price;
        if (util_.GetPrice(doc, price))
        {
            UString docid;
            if (doc.getProperty(config_.docid_property_name, docid))
            {
                std::string docid_str;
                docid.convertString(docid_str, UString::UTF_8);
                if (timestamp == -1) GetTimestamp_(doc, timestamp);
                std::map<std::string, std::string> group_prop_map;
//              task_type task = boost::bind(&ProductPriceTrend::Insert, price_trend_, docid_str, price, timestamp);
                GetGroupProperties_(doc, group_prop_map);
                task_type task = boost::bind(&ProductPriceTrend::Update, price_trend_, docid_str, price, timestamp, group_prop_map);
                jobScheduler_.addTask(task);
            }
        }
    }
    ProductClustering* clustering = GetClustering_();
    if (!clustering)
    {
        UString uuid;
        UuidGenerator::Gen(uuid);
        if (!data_source_->SetUuid(index_document, uuid)) return false;
        PMDocumentType new_doc(doc);
        doc.property(config_.uuid_property_name) = uuid;
        new_doc.property(config_.docid_property_name) = uuid;
        util_.SetItemCount(new_doc, 1);
        op_processor_->Append(1, new_doc);
    }
    else
    {
        UString uuid;
        doc.property(config_.uuid_property_name) = uuid;
        if (!data_source_->SetUuid(index_document, uuid)) return false; // set a empty uuid for rtype update later
        clustering->Insert(doc);
    }

    return true;
}

bool ProductManager::HookUpdate(PMDocumentType& to, izenelib::ir::indexmanager::IndexerDocument& index_document, time_t timestamp, bool r_type)
{
    inhook_ = true;
    boost::mutex::scoped_lock lock(human_mutex_);

    uint32_t fromid = index_document.getId(); //oldid
    PMDocumentType from;
    if (!data_source_->GetDocument(fromid, from)) return false;
    UString from_uuid;
    if (!from.getProperty(config_.uuid_property_name, from_uuid) ) return false;
    ProductPrice from_price;
    ProductPrice to_price;
    util_.GetPrice(fromid, from_price);
    util_.GetPrice(to, to_price);

    if (has_price_trend_ && to_price.Valid() && from_price != to_price)
    {
        UString docid;
        if (to.getProperty(config_.docid_property_name, docid ) )
        {
            std::string docid_str;
            docid.convertString(docid_str, UString::UTF_8);
            if (timestamp == -1) GetTimestamp_(to, timestamp);
            std::map<std::string, std::string> group_prop_map;
            GetGroupProperties_(to, group_prop_map);
            task_type task = boost::bind(&ProductPriceTrend::Update, price_trend_, docid_str, to_price, timestamp, group_prop_map);
            jobScheduler_.addTask(task);
        }
    }

    std::vector<uint32_t> docid_list;
    data_source_->GetDocIdList(from_uuid, docid_list, fromid); // except from.docid
    if (docid_list.empty()) // the from doc is unique, so delete it and insert 'to'
    {
        if (!data_source_->SetUuid(index_document, from_uuid)) return false;
        PMDocumentType new_doc(to);
        to.property(config_.uuid_property_name) = from_uuid;
        new_doc.property(config_.docid_property_name) = from_uuid;
        util_.SetItemCount(new_doc, 1);
        op_processor_->Append(2, new_doc);// if r_type, only numeric properties in 'to'
    }
    else
    {
        //need not to update(insert) to.uuid,
        if (!data_source_->SetUuid(index_document, from_uuid)) return false;
        to.property(config_.uuid_property_name) = from_uuid;
        //update price only
        if (from_price != to_price)
        {
            PMDocumentType diff_properties;
            ProductPrice price(to_price);
            util_.GetPrice(docid_list, price);
            diff_properties.property(config_.price_property_name) = price.ToUString();
            diff_properties.property(config_.docid_property_name) = from_uuid;
            //auto r_type? itemcount no need?
//                 diff_properties.property(config_.itemcount_property_name) = UString(boost::lexical_cast<std::string>(docid_list.size()+1), UString::UTF_8);

            op_processor_->Append(2, diff_properties);
        }
    }
    return true;
}

bool ProductManager::HookDelete(uint32_t docid, time_t timestamp)
{
    inhook_ = true;
    boost::mutex::scoped_lock lock(human_mutex_);
    PMDocumentType from;
    if (!data_source_->GetDocument(docid, from)) return false;
    UString from_uuid;
    if (!from.getProperty(config_.uuid_property_name, from_uuid) ) return false;
    std::vector<uint32_t> docid_list;
    data_source_->GetDocIdList(from_uuid, docid_list, docid); // except from.docid
    if (docid_list.empty()) // the from doc is unique, so delete it in A
    {
        PMDocumentType del_doc;
        del_doc.property(config_.docid_property_name) = from_uuid;
        op_processor_->Append(3, del_doc);
    }
    else
    {
        PMDocumentType diff_properties;
        ProductPrice price;
        util_.GetPrice(docid_list, price);
        diff_properties.property(config_.price_property_name) = price.ToUString();
        diff_properties.property(config_.docid_property_name) = from_uuid;
        util_.SetItemCount(diff_properties, docid_list.size());
        op_processor_->Append(2, diff_properties);
    }
    return true;
}

bool ProductManager::FinishHook()
{
    if (has_price_trend_)
    {
        task_type task = boost::bind(&ProductPriceTrend::Flush, price_trend_);
        jobScheduler_.addTask(task);
    }

    if (clustering_)
    {
        uint32_t max_in_group = 20;
        if (!clustering_->Run())
        {
            std::cout<<"ProductClustering Run failed"<<std::endl;
            return false;
        }
        typedef ProductClustering::GroupTableType GroupTableType;
        GroupTableType* group_table = clustering_->GetGroupTable();
        typedef izenelib::util::UString UuidType;
//         boost::unordered_map<GroupTableType::GroupIdType, UuidType> g2u_map;
        boost::unordered_map<GroupTableType::GroupIdType, PMDocumentType> g2doc_map;

        //output DOCID -> uuid map SCD
        ScdWriter* uuid_map_writer = NULL;
        if (!config_.uuid_map_path.empty())
        {
            boost::filesystem::create_directories(config_.uuid_map_path);
            uuid_map_writer = new ScdWriter(config_.uuid_map_path, INSERT_SCD);
        }
        const std::vector<std::vector<GroupTableType::DocIdType> >& group_info = group_table->GetGroupInfo();
        LOG(INFO)<<"Start building group info."<<std::endl;
#ifdef USE_LOG_SERVER
        LogServerConnection& conn = LogServerConnection::instance();
#endif
        for (uint32_t gid = 0; gid < group_info.size(); gid++)
        {
            std::vector<GroupTableType::DocIdType> in_group = group_info[gid];
            if (in_group.size() < 2) continue;
            if (max_in_group > 0 && in_group.size() > max_in_group) continue;
            std::sort(in_group.begin(), in_group.end());
            izenelib::util::UString docname(in_group[0], izenelib::util::UString::UTF_8);
            UuidType uuid;
#ifdef USE_LOG_SERVER
            std::string uuidstr;
            UuidGenerator::Gen(uuidstr, uuid);
#else
            UuidGenerator::Gen(uuid);
#endif
            PMDocumentType doc;
            doc.property(config_.docid_property_name) = docname;
            doc.property(config_.price_property_name) = izenelib::util::UString("", izenelib::util::UString::UTF_8);
            doc.property(config_.source_property_name) = izenelib::util::UString("", izenelib::util::UString::UTF_8);
            doc.property(config_.uuid_property_name) = uuid;
            util_.SetItemCount(doc, in_group.size());
            g2doc_map.insert(std::make_pair(gid, doc));
            if (uuid_map_writer)
            {
#ifdef USE_LOG_SERVER
                UpdateUUIDRequest uuidReq;
                uuidReq.param_.uuid_ = Utilities::uuidToUint128(uuidstr);
#endif
                for (uint32_t i = 0; i < in_group.size(); i++)
                {
                    PMDocumentType map_doc;
                    map_doc.property(config_.docid_property_name) = izenelib::util::UString(in_group[i], izenelib::util::UString::UTF_8);
                    map_doc.property(config_.uuid_property_name) = uuid;
                    uuid_map_writer->Append(map_doc);
#ifdef USE_LOG_SERVER
                    uuidReq.param_.docidList_.push_back(Utilities::md5ToUint128(in_group[i]));
#endif
                }
#ifdef USE_LOG_SERVER
                conn.asynRequest(uuidReq);
#endif
            }
        }
        LOG(INFO)<<"Finished building group info."<<std::endl;
        std::vector<std::pair<uint32_t, izenelib::util::UString> > uuid_update_list;
        uint32_t append_count = 0;
//         uint32_t has_comparison_count = 0;
#ifdef PM_PROFILER
        static double t1=0.0;
        static double t2=0.0;
        static double t3=0.0;
        static double t4=0.0;
//         std::cout<<"start buffer size : "<<common_compressed_.bufferCapacity()<<std::endl;
        izenelib::util::ClockTimer timer;
#endif
        uuid_update_list.reserve(data_source_->GetMaxDocId());
        for (uint32_t docid = 1; docid <= data_source_->GetMaxDocId(); ++docid)
        {
            if (docid % 100000 == 0)
            {
                LOG(INFO)<<"Process "<<docid<<" docs"<<std::endl;
            }
#ifdef PM_PROFILER
            if (docid % 10 == 0)
            {
                LOG(INFO)<<"PM_PROFILER : ["<<has_comparison_count<<"] "<<t1<<","<<t2<<","<<t3<<","<<t4<<std::endl;
            }
#endif
            PMDocumentType doc;
#ifdef PM_PROFILER
            timer.restart();
#endif
            if (!data_source_->GetDocument(docid, doc))
            {
                continue;
            }
#ifdef PM_PROFILER
            t1 += timer.elapsed();
#endif
            UString udocid;
            std::string sdocid;
            doc.getProperty(config_.docid_property_name, udocid);
            ProductPrice price;
            util_.GetPrice(doc, price);
            udocid.convertString(sdocid, izenelib::util::UString::UTF_8);
            GroupTableType::GroupIdType group_id;
            bool in_group = false;
            if (group_table->GetGroupId(sdocid, group_id))
            {
                if (g2doc_map.find(group_id)!=g2doc_map.end())
                {
                    in_group = true;
                }
            }

//             uint32_t itemcount = 1;
//             std::vector<std::string> docid_list_in_group;
//             bool append = true;
            if (in_group)
            {
//                 boost::unordered_map<GroupTableType::GroupIdType, UuidType>::iterator g2u_it = g2u_map.find(group_id);
                boost::unordered_map<GroupTableType::GroupIdType, PMDocumentType>::iterator g2doc_it = g2doc_map.find(group_id);
                PMDocumentType& combine_doc = g2doc_it->second;
                ProductPrice combine_price;
                util_.GetPrice(combine_doc, combine_price);
                combine_price += price;
                izenelib::util::UString base_udocid = doc.property(config_.docid_property_name).get<izenelib::util::UString>();
                UString uuid = combine_doc.property(config_.uuid_property_name).get<izenelib::util::UString>();
                UString source;
                UString combine_source;
                doc.getProperty(config_.source_property_name, source);
                combine_doc.getProperty(config_.source_property_name, combine_source);
                util_.AddSource(combine_source, source);
                if (udocid == base_udocid)
                {
                    combine_doc.copyPropertiesFromDocument(doc, false);
                }
                combine_doc.property(config_.price_property_name) = combine_price.ToUString();
                combine_doc.property(config_.source_property_name) = combine_source;
                uuid_update_list.push_back(std::make_pair(docid, uuid));
            }
            else
            {
                UString uuid;
                //not in any group
#ifdef USE_LOG_SERVER
                std::string uuidstr;
                UuidGenerator::Gen(uuidstr, uuid);
#else
                UuidGenerator::Gen(uuid);
#endif
                doc.property(config_.uuid_property_name) = uuid;
                uuid_update_list.push_back(std::make_pair(docid, uuid));
                PMDocumentType new_doc(doc);
                new_doc.property(config_.docid_property_name) = uuid;
                new_doc.eraseProperty(config_.uuid_property_name);
                util_.SetItemCount(new_doc, 1);
                op_processor_->Append(1, new_doc);
                if (uuid_map_writer)
                {
#ifdef USE_LOG_SERVER
                    UpdateUUIDRequest uuidReq;
                    uuidReq.param_.uuid_ = Utilities::uuidToUint128(uuidstr);
#endif
                    PMDocumentType map_doc;
                    map_doc.property(config_.docid_property_name) = udocid;
                    map_doc.property(config_.uuid_property_name) = uuid;
                    uuid_map_writer->Append(map_doc);
#ifdef USE_LOG_SERVER
                    uuidReq.param_.docidList_.push_back(Utilities::md5ToUint128(sdocid));
                    conn.asynRequest(uuidReq);
#endif
                }
                ++append_count;
            }
        }
        if (uuid_map_writer)
        {
            uuid_map_writer->Close();
            delete uuid_map_writer;
#ifdef USE_LOG_SERVER
        SynchronizeRequest syncReq;
        conn.asynRequest(syncReq);
#endif
        }
        //process the comparison items.
        boost::unordered_map<GroupTableType::GroupIdType, PMDocumentType>::iterator g2doc_it = g2doc_map.begin();
        while (g2doc_it != g2doc_map.end())
        {
            PMDocumentType& doc = g2doc_it->second;
            PMDocumentType new_doc(doc);
            new_doc.property(config_.docid_property_name) = new_doc.property(config_.uuid_property_name);
            new_doc.eraseProperty(config_.uuid_property_name);
            op_processor_->Append(1, new_doc);
            ++append_count;
            ++g2doc_it;
        }

        //process update_list
        LOG(INFO)<<"Total update list count : "<<uuid_update_list.size()<<std::endl;
        for (uint32_t i = 0; i < uuid_update_list.size(); i++)
        {
            if (i % 100000 == 0)
            {
                LOG(INFO)<<"Updated "<<i<<std::endl;
            }
            std::vector<uint32_t> update_docid_list(1, uuid_update_list[i].first);
            if (!data_source_->UpdateUuid(update_docid_list, uuid_update_list[i].second))
            {
                LOG(INFO)<<"UpdateUuid fail for docid : "<<uuid_update_list[i].first<<" | "<<uuid_update_list[i].second<<std::endl;
            }
        }
        data_source_->Flush();
        LOG(INFO)<<"Generate "<<append_count<<" docs for b5ma"<<std::endl;
        delete clustering_;
        clustering_ = NULL;
    }

    return GenOperations_();
}

bool ProductManager::GenOperations_()
{
    bool result = true;
    if (!op_processor_->Finish())
    {
        error_ = op_processor_->GetLastError();
        result = false;
    }
    if (inhook_) inhook_ = false;
    return result;

}

bool ProductManager::UpdateADoc(const Document& doc)
{
    if (inhook_)
    {
        error_ = "In Hook locks, collection was indexing, plz wait.";
        return false;
    }
    boost::mutex::scoped_lock lock(human_mutex_);
    if (!editor_->UpdateADoc(doc))
    {
        error_ = editor_->GetLastError();
        return false;
    }
    if (!GenOperations_())
    {
        return false;
    }
    return true;
}
bool ProductManager::AddGroup(const std::vector<uint32_t>& docid_list, PMDocumentType& info, const ProductEditOption& option)
{
    if (inhook_)
    {
        error_ = "In Hook locks, collection was indexing, plz wait.";
        return false;
    }
    boost::mutex::scoped_lock lock(human_mutex_);
    if (!editor_->AddGroup(docid_list, info, option))
    {
        error_ = editor_->GetLastError();
        return false;
    }
    if (!GenOperations_())
    {
        return false;
    }
    return true;
}




bool ProductManager::AppendToGroup(const UString& uuid, const std::vector<uint32_t>& docid_list, const ProductEditOption& option)
{
    if (inhook_)
    {
        error_ = "In Hook locks, collection was indexing, plz wait.";
        return false;
    }
    boost::mutex::scoped_lock lock(human_mutex_);
    if (!editor_->AppendToGroup(uuid, docid_list, option))
    {
        error_ = editor_->GetLastError();
        return false;
    }
    if (!GenOperations_())
    {
        return false;
    }
    return true;
}

bool ProductManager::RemoveFromGroup(const UString& uuid, const std::vector<uint32_t>& docid_list, const ProductEditOption& option)
{
    if (inhook_)
    {
        error_ = "In Hook locks, collection was indexing, plz wait.";
        return false;
    }
    boost::mutex::scoped_lock lock(human_mutex_);
    if (!editor_->RemoveFromGroup(uuid, docid_list, option))
    {
        error_ = editor_->GetLastError();
        return false;
    }
    if (!GenOperations_())
    {
        return false;
    }
    return true;
}

bool ProductManager::GetMultiPriceHistory(
        PriceHistoryList& history_list,
        const std::vector<std::string>& docid_list,
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
        const std::vector<std::string>& docid_list,
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


bool ProductManager::GetTimestamp_(const PMDocumentType& doc, time_t& timestamp) const
{
    PMDocumentType::property_const_iterator it = doc.findProperty(config_.date_property_name);
    if (it == doc.propertyEnd())
    {
        return false;
    }
    const UString& time_ustr = it->second.get<UString>();
    std::string time_str;
    time_ustr.convertString(time_str, UString::UTF_8);
    try
    {
        timestamp = Utilities::createTimeStamp(boost::posix_time::from_iso_string(time_str.insert(8, 1, 'T')));
    }
    catch (const std::exception& ex)
    {
        return false;
    }
    return true;
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
