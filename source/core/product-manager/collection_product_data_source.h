#ifndef SF1R_PRODUCTMANAGER_COLLECTIONPRODUCTDATASOURCE_H
#define SF1R_PRODUCTMANAGER_COLLECTIONPRODUCTDATASOURCE_H

#include <sstream>
#include <common/type_defs.h>


#include "pm_def.h"
#include "pm_types.h"
#include "pm_config.h"
#include "product_data_source.h"
#include <configuration-manager/PropertyConfig.h>
#include <ir/id_manager/IDManager.h>

namespace sf1r
{

class DocumentManager;
class IndexManager;
class SearchManager;

class CollectionProductDataSource : public ProductDataSource
{
public:
    CollectionProductDataSource(
            const boost::shared_ptr<DocumentManager>& document_manager,
            const boost::shared_ptr<IndexManager>& index_manager,
            const boost::shared_ptr<izenelib::ir::idmanager::IDManager>& id_manager,
            const boost::shared_ptr<SearchManager>& search_manager,
            const PMConfig& config,
            const IndexBundleSchema& indexSchema);

    ~CollectionProductDataSource();

    uint32_t GetMaxDocId() const;

    bool GetDocument(uint32_t docid, PMDocumentType& doc);

    void GetRTypePropertiesForDocument(uint32_t docid, PMDocumentType& doc);

    void GetDocIdList(const izenelib::util::UString& uuid, std::vector<uint32_t>& docid_list, uint32_t exceptid);

    bool UpdateUuid(const std::vector<uint32_t>& docid_list, const izenelib::util::UString& uuid);

    bool SetUuid(izenelib::ir::indexmanager::IndexerDocument& doc, const izenelib::util::UString& uuid);

    bool GetInternalDocidList(const std::vector<uint128_t>& sdocid_list, std::vector<uint32_t>& docid_list);

    bool AddCurUuidToHistory(uint32_t docid);

    bool GetPrice(const PMDocumentType& doc, ProductPrice& price) const;

    bool GetPrice(const uint32_t& doc, ProductPrice& price) const;

    void Flush();

private:
    boost::shared_ptr<DocumentManager> document_manager_;
    boost::shared_ptr<IndexManager> index_manager_;
    boost::shared_ptr<izenelib::ir::idmanager::IDManager> id_manager_;
    boost::shared_ptr<SearchManager> search_manager_;
    PMConfig config_;
    IndexBundleSchema indexSchema_;
};

}

#endif
