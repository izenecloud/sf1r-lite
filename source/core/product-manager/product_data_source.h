#ifndef SF1R_PRODUCTMANAGER_PRODUCTDATASOURCE_H
#define SF1R_PRODUCTMANAGER_PRODUCTDATASOURCE_H

#include <sstream>
#include <common/type_defs.h>


#include "pm_def.h"
#include "pm_types.h"
#include <ir/index_manager/index/IndexerDocument.h>


namespace sf1r
{

class ProductDataSource
{
public:

    virtual ~ProductDataSource() {}

    virtual bool GetDocument(uint32_t docid, PMDocumentType& doc) { return false;}

    virtual void GetDocIdList(const izenelib::util::UString& uuid, std::vector<uint32_t>& docid_list, uint32_t exceptid) {}

    virtual bool UpdateUuid(const std::vector<uint32_t>& docid_list, const izenelib::util::UString& uuid) {return false;}

    virtual bool SetUuid(izenelib::ir::indexmanager::IndexerDocument& doc, const izenelib::util::UString& uuid) {return false;}

    virtual bool GetInternalDocidList(const std::vector<izenelib::util::UString>& sdocid_list, std::vector<uint32_t>& docid_list) {return false;}


    const std::string& GetLastError()
    {
        return error_;
    }

protected:
    void SetError_(const std::string& error)
    {
        error_ = error;
    }

private:
    std::string error_;

};

}

#endif
