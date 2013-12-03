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

bool ProductIndexHooker::HookInsert(const Document& doc, time_t timestamp)
{
    return product_manager_->HookInsert(doc, timestamp);
}

bool ProductIndexHooker::HookUpdate(const Document& doc, docid_t oldid, time_t timestamp)
{
    return product_manager_->HookUpdate(doc, oldid, timestamp);
}

bool ProductIndexHooker::HookDelete(docid_t docid, time_t timestamp)
{
    return product_manager_->HookDelete(docid, timestamp);
}

bool ProductIndexHooker::FinishHook(time_t timestamp)
{
    return product_manager_->FinishHook(timestamp);
}
