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
class ProductBackup;

class ProductManager
{
public:
    ProductManager(ProductDataSource* data_source, OperationProcessor* op_processor, const PMConfig& config);

    ~ProductManager();

    bool Recover();


    bool HookInsert(PMDocumentType& doc, izenelib::ir::indexmanager::IndexerDocument& index_document);

    bool HookUpdate(PMDocumentType& to, izenelib::ir::indexmanager::IndexerDocument& index_document, bool r_type);

    bool HookDelete(uint32_t docid);

    bool GenOperations();


    //update documents in A, so need transfer
    bool UpdateADoc(const Document& doc, bool backup = true);

    //all intervention functions.
    bool AddGroup(const std::vector<uint32_t>& docid_list, izenelib::util::UString& gen_uuid, bool backup = true);

    bool AppendToGroup(const izenelib::util::UString& uuid, const std::vector<uint32_t>& docid_list, bool backup = true);

    bool RemoveFromGroup(const izenelib::util::UString& uuid, const std::vector<uint32_t>& docid_list, bool backup = true);

    bool AddGroupWithInfo(const std::vector<uint32_t>& docid_list, const Document& doc, bool backup = true);

    bool AddGroupWithInfo(const std::vector<izenelib::util::UString>& docid_list, const Document& doc, bool backup = true);

    typedef std::vector<std::pair<izenelib::util::UString, std::pair<ProductPriceType, ProductPriceType> > > PriceHistoryList;
    bool GetPriceHistory(
            std::map<uint32_t, PriceHistoryList>& history_map,
            const std::vector<uint32_t>& docid_list,
            const izenelib::util::UString& from_time,
            const izenelib::util::UString& to_time);

    bool GetPriceRange(
            std::map<uint32_t, std::pair<ProductPriceType, ProductPriceType> >& range_map,
            const std::vector<uint32_t>& docid_list,
            const izenelib::util::UString& from_time,
            const izenelib::util::UString& to_time);


    const std::string& GetLastError() const
    {
        return error_;
    }

    const PMConfig& GetConfig() const
    {
        return config_;
    }

private:

    void BackupPCItem_(const izenelib::util::UString& uuid, const std::vector<uint32_t>& docid_list, int type);

    bool UpdateADoc_(const Document& doc);

    bool AppendToGroup_(const izenelib::util::UString& uuid, const std::vector<uint32_t>& uuid_docid_list, const std::vector<uint32_t>& docid_list, const PMDocumentType& uuid_doc);

    bool GetPrice_(uint32_t docid, ProductPrice& price) const;

    bool GetPrice_(const PMDocumentType& doc, ProductPrice& price) const;

    void GetPrice_(const std::vector<uint32_t>& docid_list, ProductPrice& price) const;

    bool GetUuid_(const PMDocumentType& doc, izenelib::util::UString& uuid) const;

    bool GetDOCID_(const PMDocumentType& doc, izenelib::util::UString& docid) const;

    void SetItemCount_(PMDocumentType& doc, uint32_t item_count);

    bool UpdatePriceHistory_(const PMDocumentType& doc) const;

private:
    ProductDataSource* data_source_;
    OperationProcessor* op_processor_;
    UuidGenerator* uuid_gen_;
    ProductBackup* backup_;
    PMConfig config_;
    std::string error_;
    bool inhook_;
    boost::mutex human_mutex_;
};

}

#endif
