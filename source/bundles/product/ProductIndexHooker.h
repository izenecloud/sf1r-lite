#ifndef SF1R_PRODUCT_BUNDLE_PRODUCTINDEXHOOKER_H
#define SF1R_PRODUCT_BUNDLE_PRODUCTINDEXHOOKER_H


#include <document-manager/Document.h>
#include <ir/index_manager/index/IndexerDocument.h>
#include <index-manager/IndexHooker.h>


#include <boost/shared_ptr.hpp>

namespace sf1r
{
class ProductManager;

class ProductIndexHooker : public IndexHooker
{
public:
    ProductIndexHooker(const boost::shared_ptr<ProductManager>& product_manager);
    ~ProductIndexHooker();
    bool HookInsert(Document& doc, izenelib::ir::indexmanager::IndexerDocument& index_document);
    bool HookUpdate(Document& doc, izenelib::ir::indexmanager::IndexerDocument& index_document, bool r_type);
    bool HookDelete(docid_t docid);
    bool Finish();
    
private:
    boost::shared_ptr<ProductManager> product_manager_;
    
};


}

#endif
