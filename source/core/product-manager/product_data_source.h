#ifndef SF1R_PRODUCTMANAGER_PRODUCTDATASOURCE_H
#define SF1R_PRODUCTMANAGER_PRODUCTDATASOURCE_H

#include <sstream>
#include <common/type_defs.h>


#include "pm_def.h"
#include "pm_types.h"


namespace sf1r
{

class ProductDataSource
{
public:
    
    virtual ~ProductDataSource() {}
    
    virtual bool GetDocument(uint32_t docid, PMDocumentType& doc) { return false;}
    
    virtual void GetDocIdList(const izenelib::util::UString& uuid, std::vector<uint32_t>& docid_list, uint32_t exceptid) {}
    
};

}

#endif

