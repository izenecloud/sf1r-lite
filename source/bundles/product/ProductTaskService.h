#ifndef PRODUCT_BUNDLE_TASK_SERVICE_H
#define PRODUCT_BUNDLE_TASK_SERVICE_H

#include "ProductBundleConfiguration.h"

#include <directory-manager/DirectoryRotator.h>
#include <document-manager/Document.h>
#include <document-manager/text-summarization-submanager/TextSummarizationSubManager.h>
#include <configuration-manager/PropertyConfig.h>
#include <configuration-manager/ConfigurationTool.h>
#include <common/Status.h>
#include <common/IndexingProgress.h>
#include <common/ScdParser.h>

#include <ir/id_manager/IDManager.h>

#include <util/osgi/IService.h>
#include <util/driver/Value.h>

#include <boost/shared_ptr.hpp>

namespace sf1r
{
using izenelib::ir::idmanager::IDManager;

class IndexTaskService;
class IndexManager;
class DocumentManager;
class ProductTaskService : public ::izenelib::osgi::IService
{
    typedef uint32_t CharacterOffset;
public:
    ProductTaskService(
        ProductBundleConfiguration* bundleConfig);

    ~ProductTaskService();

    bool buildCollection(unsigned int numdoc);

private:
    bool doBuildCollection_(
        const std::string& scdFile, 
        int op, 
        uint32_t numdoc
    );
    
    bool backup_();

    static void value2SCDDoc(
        const ::izenelib::driver::Value& value, 
        SCDDoc& scddoc
    );

private:
    ProductBundleConfiguration* bundleConfig_;
    DirectoryRotator directoryRotator_;
    IndexTaskService* indexTaskService_;

    std::string productSourceField_;
    PropertyConfig dateProperty_;
    config_tool::PROPERTY_ALIAS_MAP_T propertyAliasMap_;

    boost::shared_ptr<IDManager> idManager_;
    boost::shared_ptr<DocumentManager> documentManager_;
    boost::shared_ptr<IndexManager> indexManager_;
    TextSummarizationSubManager summarizer_;

    friend class ProductBundleActivator;
};

}

#endif

