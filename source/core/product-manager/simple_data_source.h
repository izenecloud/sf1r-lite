#ifndef SF1R_PRODUCTMANAGER_SIMPLEDATASOURCE_H
#define SF1R_PRODUCTMANAGER_SIMPLEDATASOURCE_H

#include <sstream>
#include <common/type_defs.h>
#include <boost/unordered_map.hpp>

#include "pm_def.h"
#include "pm_types.h"
#include "pm_config.h"
#include "product_data_source.h"
#include <document-manager/Document.h>

namespace sf1r
{

class SimpleDataSource : public ProductDataSource
{
    typedef boost::unordered_map<std::string, std::vector<uint32_t> > UuidIndexType;

public:

    SimpleDataSource(const PMConfig& config, std::vector<PMDocumentType>* document_list);

    ~SimpleDataSource();

    bool GetDocument(uint32_t docid, PMDocumentType& doc);

    void GetDocIdList(const izenelib::util::UString& uuid, std::vector<uint32_t>& docid_list, uint32_t exceptid);

    bool UpdateUuid(const std::vector<uint32_t>& docid_list, const izenelib::util::UString& uuid);

    bool SetUuid(izenelib::ir::indexmanager::IndexerDocument& doc, const izenelib::util::UString& uuid);

    bool AddDocument(uint32_t docid, const PMDocumentType& doc);

    bool UpdateDocument(uint32_t docid, const PMDocumentType& doc);

    bool DeleteDocument(uint32_t docid);

    bool GetPrice(const PMDocumentType& doc, ProductPrice& price) const;

    bool GetPrice(const uint32_t& docid, ProductPrice& price) const;

private:
    void RebuildIndex_();

    bool GetUuid_(uint32_t docid, izenelib::util::UString& uuid);

    bool GetUuid_(uint32_t docid, std::string& suuid);

    template <typename T>
    void VectorRemove_(std::vector<T>& vec, const T& value)
    {
        vec.erase( std::remove(vec.begin(), vec.end(), value),vec.end());
    }

private:
    PMConfig config_;
    std::vector<PMDocumentType>* document_list_;
    UuidIndexType uuid_index_;
};

}

#endif
