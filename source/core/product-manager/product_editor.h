#ifndef SF1R_PRODUCTMANAGER_PRODUCTEDITOR_H
#define SF1R_PRODUCTMANAGER_PRODUCTEDITOR_H

#include <common/type_defs.h>

#include "pm_def.h"
#include "pm_types.h"
#include "pm_config.h"
#include "product_price.h"
#include "pm_util.h"

namespace sf1r
{

class ProductDataSource;
class OperationProcessor;

class ProductEditor
{
public:
    
    
    
    
    ProductEditor(
            ProductDataSource* data_source,
            OperationProcessor* op_processor,
            const PMConfig& config);

    ~ProductEditor();

    //update documents in A, so need transfer
    bool UpdateADoc(const Document& doc);

    //all intervention functions.
    bool AddGroup(const std::vector<uint32_t>& docid_list, PMDocumentType& info, const ProductEditOption& option);

    bool AppendToGroup(const izenelib::util::UString& uuid, const std::vector<uint32_t>& docid_list, const ProductEditOption& option);

    bool RemoveFromGroup(const izenelib::util::UString& uuid, const std::vector<uint32_t>& docid_list, const ProductEditOption& option);

    inline const std::string& GetLastError() const
    {
        return error_;
    }

    inline const PMConfig& GetConfig() const
    {
        return config_;
    }

private:
    
    bool AppendToGroup_(const std::vector<PMDocumentType>& doc_list, const PMDocumentType& info);
    

private:
    ProductDataSource* data_source_;
    OperationProcessor* op_processor_;

    PMConfig config_;
    PMUtil util_;
    std::string error_;
};

}

#endif
