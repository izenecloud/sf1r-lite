#ifndef SF1R_PRODUCTMANAGER_PRODUCTMANAGER_H
#define SF1R_PRODUCTMANAGER_PRODUCTMANAGER_H

#include <common/type_defs.h>

#include <document-manager/Document.h>
#include <ir/index_manager/index/IndexerDocument.h>
#include <idmlib/duplicate-detection/dup_detector.h>
#include "pm_def.h"
#include "pm_types.h"
#include "pm_config.h"
#include "pm_util.h"
#include "product_price.h"
#include <boost/shared_ptr.hpp>
#include <boost/thread/mutex.hpp>

namespace sf1r
{

class ProductDataSource;
class OperationProcessor;
class ProductEditor;
class ProductPriceTrend;
class ProductClustering;
class ProductClusteringPostItem;

class ProductManager
{
public:
    
   
    ProductManager(
            const std::string& work_dir,
            ProductDataSource* data_source,
            OperationProcessor* op_processor,
            ProductPriceTrend* price_trend,
            const PMConfig& config);

    ~ProductManager();

    bool Recover();

    bool HookInsert(PMDocumentType& doc, izenelib::ir::indexmanager::IndexerDocument& index_document, time_t timestamp);

    bool HookUpdate(PMDocumentType& to, izenelib::ir::indexmanager::IndexerDocument& index_document, time_t timestamp, bool r_type);

    bool HookDelete(uint32_t docid, time_t timestamp);

    bool FinishHook();

    //update documents in A, so need transfer
    bool UpdateADoc(const Document& doc);

    //all intervention functions.
    bool AddGroup(const std::vector<uint32_t>& docid_list, PMDocumentType& info, const ProductEditOption& option);

    bool AppendToGroup(const UString& uuid, const std::vector<uint32_t>& docid_list, const ProductEditOption& option);

    bool RemoveFromGroup(const UString& uuid, const std::vector<uint32_t>& docid_list, const ProductEditOption& option);


    bool GetMultiPriceHistory(
            PriceHistoryList& history_list,
            const std::vector<std::string>& docid_list,
            time_t from_tt,
            time_t to_tt);

    bool GetMultiPriceRange(
            PriceRangeList& range_list,
            const std::vector<std::string>& docid_list,
            time_t from_tt,
            time_t to_tt);

    bool GetTopPriceCutList(
            TPCQueue& tpc_queue,
            const std::string& prop_name,
            const std::string& prop_value,
            uint32_t days,
            uint32_t count);

    inline const std::string& GetLastError() const
    {
        return error_;
    }

    inline const PMConfig& GetConfig() const
    {
        return config_;
    }

private:
    ProductClustering* GetClustering_();
    
        
    bool GenOperations_();

    
    bool GetTimestamp_(const PMDocumentType& doc, time_t& timestamp) const;

    bool GetGroupProperties_(const PMDocumentType& doc, std::map<std::string, std::string>& group_prop_map) const;
    
    bool GetCategory_(const PMDocumentType& doc, izenelib::util::UString& category);
    

private:
    std::string work_dir_;
    PMConfig config_;
    ProductDataSource* data_source_;
    OperationProcessor* op_processor_;
    ProductPriceTrend* price_trend_;
    ProductClustering* clustering_;
    ProductEditor* editor_;
    PMUtil util_;
    std::string error_;
    bool has_price_trend_;
    bool inhook_;
    boost::mutex human_mutex_;
};

}

#endif
