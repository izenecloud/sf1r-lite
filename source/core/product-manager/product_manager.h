#ifndef SF1R_PRODUCTMANAGER_PRODUCTMANAGER_H
#define SF1R_PRODUCTMANAGER_PRODUCTMANAGER_H

#include <common/type_defs.h>

#include <document-manager/Document.h>
#include "pm_def.h"
#include "pm_types.h"
#include "pm_config.h"
#include "product_price.h"

#include <common/JobScheduler.h>

#include <boost/shared_ptr.hpp>
#include <boost/thread/mutex.hpp>

namespace sf1r
{

class DocumentManager;
class ProductDataSource;
class ProductPriceTrend;
class PMUtil;

class ProductManager
{
public:


    ProductManager(
            const std::string& work_dir,
            const boost::shared_ptr<DocumentManager>& document_manager,
            ProductDataSource* data_source,
            ProductPriceTrend* price_trend,
            const PMConfig& config);

    ~ProductManager();

    bool HookInsert(const PMDocumentType& doc, time_t timestamp);

    bool HookUpdate(const PMDocumentType& to, docid_t oldid, time_t timestamp);

    bool HookDelete(uint32_t docid, time_t timestamp);

    bool FinishHook(time_t timestamp);

    bool GetMultiPriceHistory(
            PriceHistoryList& history_list,
            const std::vector<uint128_t>& docid_list,
            time_t from_tt,
            time_t to_tt);

    bool GetMultiPriceRange(
            PriceRangeList& range_list,
            const std::vector<uint128_t>& docid_list,
            time_t from_tt,
            time_t to_tt);

    bool GetTopPriceCutList(
            TPCQueue& tpc_queue,
            const std::string& prop_name,
            const std::string& prop_value,
            uint32_t days,
            uint32_t count);

    bool MigratePriceHistory(
            const std::string& new_keyspace,
            uint32_t start);

    inline const std::string& GetLastError() const
    {
        return error_;
    }

    inline const PMConfig& GetConfig() const
    {
        return config_;
    }

private:
    bool GetTimestamp_(const PMDocumentType& doc, time_t& timestamp) const;

    bool GetGroupProperties_(const PMDocumentType& doc, std::map<std::string, std::string>& group_prop_map) const;

    bool GetCategory_(const PMDocumentType& doc, izenelib::util::UString& category);


private:
    std::string work_dir_;
    PMConfig config_;
    boost::shared_ptr<DocumentManager> document_manager_;
    ProductDataSource* data_source_;
    ProductPriceTrend* price_trend_;
    PMUtil* util_;
    std::string error_;

    bool has_price_trend_;
    JobScheduler jobScheduler_;

    bool inhook_;
    boost::mutex human_mutex_;
};

}

#endif
