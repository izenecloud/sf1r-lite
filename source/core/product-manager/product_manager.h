#ifndef SF1R_PRODUCTMANAGER_PRODUCTMANAGER_H
#define SF1R_PRODUCTMANAGER_PRODUCTMANAGER_H

#include <common/type_defs.h>


#include <document-manager/Document.h>

#include "pm_def.h"
#include "pm_types.h"
#include "pm_config.h"
#include "product_price.h"
#include <boost/shared_ptr.hpp>


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
    
    
    bool HookInsert(PMDocumentType& doc);
    
    bool HookUpdate(uint32_t fromid, PMDocumentType& to, bool r_type);
    
    bool HookDelete(uint32_t docid);
    
    void GenOperations();
    
private:
    
    bool HookUpdateNew_(uint32_t fromid, PMDocumentType& to);
    
    bool GetPrice_(uint32_t docid, ProductPrice& price);
    
    void GetPrice_(const std::vector<uint32_t>& docid_list, ProductPrice& price);
    
    bool GetUuid_(const PMDocumentType& doc, izenelib::util::UString& uuid);
    
private:
    ProductDataSource* data_source_;
    OperationProcessor* op_processor_;
    UuidGenerator* uuid_gen_;
    PMConfig config_;

};

}

#endif

