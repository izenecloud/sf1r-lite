#include "ProductIndexHooker.h"
#include <product-manager/product_manager.h>

using namespace sf1r;

ProductIndexHooker::ProductIndexHooker(const boost::shared_ptr<ProductManager>& product_manager)
:product_manager_(product_manager)
{

}

ProductIndexHooker::~ProductIndexHooker()
{

}

bool ProductIndexHooker::HookInsert(Document& doc, izenelib::ir::indexmanager::IndexerDocument& index_document)
{
    return product_manager_->HookInsert(doc, index_document);
}

bool ProductIndexHooker::HookUpdate(Document& doc, izenelib::ir::indexmanager::IndexerDocument& index_document, bool r_type)
{
    return product_manager_->HookUpdate(doc, index_document, r_type);
}

// bool ProductIndexHooker::HookInsert(Document& doc)
// {
//     return product_manager_->HookInsert(doc);
// }
//
// bool ProductIndexHooker::HookUpdate(docid_t oldid, Document& doc, bool r_type)
// {
//     return product_manager_->HookUpdate(oldid, doc, r_type);
// }

bool ProductIndexHooker::HookDelete(docid_t docid)
{
    return product_manager_->HookDelete(docid);
}

bool ProductIndexHooker::Finish()
{
    return product_manager_->GenOperations();
}
