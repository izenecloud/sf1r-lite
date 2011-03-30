#ifndef INDEX_BUNDLE_TASK_SERVICE_H
#define INDEX_BUNDLE_TASK_SERVICE_H

#include <index-manager/IndexManager.h>
#include <document-manager/Document.h>
#include <document-manager/DocumentManager.h>
#include <document-manager/text-summarization-submanager/TextSummarizationSubManager.h>
#include <la-manager/LAManager.h>

#include <ir/id_manager/IDManager.h>

#include <util/osgi/IService.h>

#include <boost/shared_ptr.hpp>

namespace sf1r
{
using izenelib::ir::idmanager::IDManager;
class IndexTaskService : public ::izenelib::osgi::IService
{
public:
    IndexTaskService();

    ~IndexTaskService();

    void buildCollection(unsigned int numdoc);

    void optimizeIndex();
private:

    boost::shared_ptr<LAManager> laManager_;
    boost::shared_ptr<IDManager> idManager_;
    boost::shared_ptr<DocumentManager> documentManager_;
    boost::shared_ptr<IndexManager> indexManager_;
    TextSummarizationSubManager summarizer_;

    friend class IndexBundleActivator;
};

}

#endif

