#ifndef INDEX_BUNDLE_TASK_SERVICE_H
#define INDEX_BUNDLE_TASK_SERVICE_H

#include "IndexBundleConfiguration.h"

#include <directory-manager/DirectoryRotator.h>
#include <document-manager/Document.h>
#include <document-manager/text-summarization-submanager/TextSummarizationSubManager.h>
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
class RecommendTaskService;
class IndexManager;
class DocumentManager;
class LAManager;
class SearchManager;
class IndexTaskService : public ::izenelib::osgi::IService
{
    typedef uint32_t CharacterOffset;
public:
    IndexTaskService(
        IndexBundleConfiguration* bundleConfig, 
        DirectoryRotator& directoryRotator,
        boost::shared_ptr<IndexManager> indexManager);

    ~IndexTaskService();

    bool buildCollection(unsigned int numdoc);

    bool optimizeIndex();

    bool createDocument(const ::izenelib::driver::Value& documentValue);

    bool updateDocument(const ::izenelib::driver::Value& documentValue);

    bool destroyDocument(const ::izenelib::driver::Value& documentValue);

    bool getIndexStatus(Status& status);

    uint32_t getDocNum();

private:
    bool doBuildCollection_(
        const std::string& scdFile, 
        int op, 
        uint32_t numdoc
    );
    
    bool prepareDocument_(
        SCDDoc& doc,
        Document& document,
        IndexerDocument& indexDocument,
        bool& rType,
        std::map<std::string, pair<PropertyDataType, izenelib::util::UString> >& rTypeFieldValue,
        std::string& source,
        bool insert = true
    );

    bool checkSeparatorType_(
        const izenelib::util::UString& propertyValueStr,
        izenelib::util::UString::EncodingType encoding,
        char separator
    );

    bool preparePartialDocument_(
        Document& document, 
        IndexerDocument& oldIndexDocument
    );

    void checkRtype_(
        SCDDoc& doc,
        bool& rType,
        std::map<std::string, pair<PropertyDataType, izenelib::util::UString> >& rTypeFieldValue
    );

    bool makeSentenceBlocks_(
        const izenelib::util::UString& text,
        const unsigned int numOfSummary,
        const unsigned int maxDisplayLength,
        std::vector<CharacterOffset>& sentenceOffsetList
    );

    bool makeForwardIndex_(
        const izenelib::util::UString& text,
        const std::string& propertyName,
        unsigned int propertyId,
        const AnalysisInfo& analysisInfo
    );

    bool backup_();

    static void value2SCDDoc(
        const ::izenelib::driver::Value& value, 
        SCDDoc& scddoc
    );

private:
    IndexBundleConfiguration* bundleConfig_;
    DirectoryRotator& directoryRotator_;
    MiningTaskService* miningTaskService_;
    RecommendTaskService* recommendTaskService_;

    PropertyConfig dateProperty_;
    config_tool::PROPERTY_ALIAS_MAP_T propertyAliasMap_;

    boost::shared_ptr<LAManager> laManager_;
    boost::shared_ptr<IDManager> idManager_;
    boost::shared_ptr<DocumentManager> documentManager_;
    boost::shared_ptr<IndexManager> indexManager_;
    TextSummarizationSubManager summarizer_;
    boost::shared_ptr<SearchManager> searchManager_;

    unsigned int collectionId_;
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

