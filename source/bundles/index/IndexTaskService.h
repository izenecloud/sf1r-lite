#ifndef INDEX_BUNDLE_TASK_SERVICE_H
#define INDEX_BUNDLE_TASK_SERVICE_H

#include "IndexBundleConfiguration.h"

#include <aggregator-manager/IndexAggregator.h>
#include <directory-manager/DirectoryRotator.h>
#include <common/Status.h>
#include <common/ScdParser.h>

#include <util/driver/Value.h>
#include <util/osgi/IService.h>

#include <boost/shared_ptr.hpp>

namespace sf1r
{
using izenelib::ir::idmanager::IDManager;

class IndexWorker;
class DocumentManager;
class ScdSharder;

class IndexTaskService : public ::izenelib::osgi::IService
{
public:
    IndexTaskService(IndexBundleConfiguration* bundleConfig);

    ~IndexTaskService();


    bool index(unsigned int numdoc);

    bool index(boost::shared_ptr<DocumentManager>& documentManager);

    bool optimizeIndex();

    bool createDocument(const ::izenelib::driver::Value& documentValue);

    bool updateDocument(const ::izenelib::driver::Value& documentValue);
    bool updateDocumentInplace(const ::izenelib::driver::Value& request);

    bool destroyDocument(const ::izenelib::driver::Value& documentValue);

    bool getIndexStatus(Status& status);

    bool isAutoRebuild();

    uint32_t getDocNum();

    uint32_t getKeyCount(const std::string& property_name);

    std::string getScdDir() const;

    CollectionPath&  getCollectionPath() const;

    boost::shared_ptr<DocumentManager> getDocumentManager() const;

    void flush();
    bool reload();

private:
    bool HookDistributeRequestForIndex();
    bool distributedIndex_(unsigned int numdoc);
    bool distributedIndexImpl_(
        unsigned int numdoc,
        const std::string& collectionName,
        const std::string& masterScdPath,
        const std::vector<std::string>& shardKeyList);

    bool createScdSharder(
        boost::shared_ptr<ScdSharder>& scdSharder,
        const std::vector<std::string>& shardKeyList);

private:
    std::string service_;
    IndexBundleConfiguration* bundleConfig_;

    boost::shared_ptr<IndexAggregator> indexAggregator_;
    boost::shared_ptr<IndexWorker> indexWorker_;

    friend class IndexWorkerController;
    friend class CollectionTaskScheduler;
    friend class IndexBundleActivator;
    friend class ProductBundleActivator;
};

}

#endif
