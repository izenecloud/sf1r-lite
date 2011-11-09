#ifndef PRODUCT_BUNDLE_SEARCH_SERVICE_H
#define PRODUCT_BUNDLE_SEARCH_SERVICE_H

#include "ProductBundleConfiguration.h"

#include <common/sf1_serialization_types.h>
#include <common/type_defs.h>
#include <common/Status.h>

#include <document-manager/Document.h>

#include <util/osgi/IService.h>
#include <boost/shared_ptr.hpp>


namespace sf1r
{
class ProductManager;

class ProductSearchService : public ::izenelib::osgi::IService
{
public:
    ProductSearchService(ProductBundleConfiguration* config);

    ~ProductSearchService();

    boost::shared_ptr<ProductManager> GetProductManager()
    {
        return productManager_;
    }

public:

private:
    ProductBundleConfiguration* bundleConfig_;
    boost::shared_ptr<ProductManager> productManager_;

    friend class ProductBundleActivator;
};

}

#endif
