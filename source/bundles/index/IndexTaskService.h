#ifndef INDEX_BUNDLE_TASK_SERVICE_H
#define INDEX_BUNDLE_TASK_SERVICE_H

#include "IndexBundleConfiguration.h"

#include <directory-manager/DirectoryRotator.h>
#include <index-manager/IndexManager.h>
#include <document-manager/Document.h>
#include <document-manager/DocumentManager.h>
#include <document-manager/text-summarization-submanager/TextSummarizationSubManager.h>
#include <la-manager/LAManager.h>
#include <search-manager/SearchManager.h>
#include <configuration-manager/PropertyConfig.h>
#include <configuration-manager/ConfigurationTool.h>
#include <common/Status.h>
#include <common/IndexingProgress.h>
#include <common/ScdParser.h>

#include <ir/id_manager/IDManager.h>
#include <ir/index_manager/index/IndexerDocument.h>

#include <util/osgi/IService.h>
#include <util/driver/Value.h>

#include <boost/shared_ptr.hpp>

namespace sf1r
{
using izenelib::ir::idmanager::IDManager;

class MiningTaskService;
class IndexTaskService : public ::izenelib::osgi::IService
{
    typedef uint32_t CharacterOffset;
public:
    IndexTaskService(IndexBundleConfiguration* bundleConfig, DirectoryRotator& directoryRotator);

    ~IndexTaskService();

    bool buildCollection(unsigned int numdoc);

    bool optimizeIndex();

    bool createDocument(const ::izenelib::driver::Value& documentValue);

    bool updateDocument(const ::izenelib::driver::Value& documentValue);

    bool destroyDocument(const ::izenelib::driver::Value& documentValue);

    bool getIndexStatus(Status& status);

private:
    bool doBuildCollection_(const std::string& scdFile, int op, uint32_t numdoc);
    
    bool prepareDocument_(SCDDoc& doc,
                          Document& document,
                          IndexerDocument& indexDocument,
                          bool insert = true);

    bool makeSentenceBlocks_(const izenelib::util::UString& text,
                             unsigned int propertyId,
                             const unsigned int numOfSummary,
                             const unsigned int maxDisplayLength,
                             std::vector<CharacterOffset>& sentenceOffsetList);

    bool makeForwardIndex_(const izenelib::util::UString& text,
                           unsigned int propertyId,
                           const AnalysisInfo& analysisInfo);

    bool backup_();

    static void value2SCDDoc(const ::izenelib::driver::Value& value, SCDDoc& scddoc);

private:
    IndexBundleConfiguration* bundleConfig_;
    DirectoryRotator& directoryRotator_;
    MiningTaskService* miningTaskService_;

    unsigned int collectionId_;

    PropertyConfig dateProperty_;
    config_tool::PROPERTY_ALIAS_MAP_T propertyAliasMap_;

    boost::shared_ptr<LAManager> laManager_;
    boost::shared_ptr<IDManager> idManager_;
    boost::shared_ptr<DocumentManager> documentManager_;
    boost::shared_ptr<IndexManager> indexManager_;
    TextSummarizationSubManager summarizer_;
    boost::shared_ptr<SearchManager> searchManager_;

    docid_t maxDocId_;
    IndexingProgress indexProgress_;
    bool checkInsert_;
    unsigned int numDeletedDocs_;
    unsigned int numUpdatedDocs_;

    Status indexStatus_;

    std::vector<boost::shared_ptr<LAInput> > laInputs_;

    friend class IndexBundleActivator;
};

}

#endif

