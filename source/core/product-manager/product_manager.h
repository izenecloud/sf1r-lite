#ifndef SF1R_PRODUCTMANAGER_PRODUCTMANAGER_H
#define SF1R_PRODUCTMANAGER_PRODUCTMANAGER_H

#include <common/type_defs.h>


#include <document-manager/Document.h>
#include <ir/index_manager/index/IndexerDocument.h>

#include "pm_def.h"
#include "pm_types.h"
#include "pm_config.h"
#include "product_price.h"
#include <boost/shared_ptr.hpp>
#include <boost/thread/mutex.hpp>

namespace sf1r
{
    
class ProductDataSource;
class OperationProcessor;
class UuidGenerator;

class ProductManager
{
public:
    ProductManager(ProductDataSource* data_source, OperationProcessor* op_processor, const PMConfig& config);

    ~ProductManager();
    
    
    bool HookInsert(PMDocumentType& doc, izenelib::ir::indexmanager::IndexerDocument& index_document);
    
    bool HookUpdate(PMDocumentType& to, izenelib::ir::indexmanager::IndexerDocument& index_document, bool r_type);
    
    bool HookDelete(uint32_t docid);
    
    bool GenOperations();
    
    const std::string& GetLastError() const
    {
        return error_;
    }
    
    //all intervention functions.
    bool AddGroup(const std::vector<uint32_t>& docid_list, izenelib::util::UString& gen_uuid);
    
    bool AppendToGroup(const izenelib::util::UString& uuid, const std::vector<uint32_t>& docid_list);
    
    bool RemoveFromGroup(const izenelib::util::UString& uuid, const std::vector<uint32_t>& docid_list);
    
private:
    
    bool AppendToGroup_(const izenelib::util::UString& uuid, const std::vector<uint32_t>& docid_list);
    
    bool GetPrice_(uint32_t docid, ProductPrice& price);
    
    bool GetPrice_(const PMDocumentType& doc, ProductPrice& price);
    
    void GetPrice_(const std::vector<uint32_t>& docid_list, ProductPrice& price);
    
    bool GetUuid_(const PMDocumentType& doc, izenelib::util::UString& uuid);
    
    bool GetDOCID_(const PMDocumentType& doc, izenelib::util::UString& docid);
    
    void SetItemCount_(PMDocumentType& doc, uint32_t item_count);
    
private:
    ProductDataSource* data_source_;
    OperationProcessor* op_processor_;
    UuidGenerator* uuid_gen_;
    PMConfig config_;
    std::string error_;
    bool inhook_;
    boost::mutex human_mutex_;
};

}

#endif

