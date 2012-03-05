/**
 * @file IndexWorker.h
 * @author Zhongxia Li
 * @date Dec 5, 2011
 * @brief Index task worker
 */
#ifndef INDEX_WORKER_H_
#define INDEX_WORKER_H_

#include <directory-manager/DirectoryRotator.h>
#include <configuration-manager/PropertyConfig.h>
#include <configuration-manager/ConfigurationTool.h>
#include <document-manager/Document.h>
#include <document-manager/text-summarization-submanager/TextSummarizationSubManager.h>
#include <common/Status.h>
#include <common/IndexingProgress.h>
#include <common/ScdParser.h>
#include <common/ScdWriterController.h>

#include <ir/id_manager/IDManager.h>
#include <ir/index_manager/index/IndexerDocument.h>

#include <util/driver/Value.h>

#include <boost/shared_ptr.hpp>
#include <boost/variant/get.hpp>

namespace sf1r{
using izenelib::ir::idmanager::IDManager;

class IndexBundleConfiguration;
class MiningTaskService;
class RecommendTaskService;
class IndexManager;
class DocumentManager;
class LAManager;
class SearchManager;
class MiningManager;
class ScdWriterController;
class IndexHooker;
class SearchWorker;

class IndexWorker
{
    typedef uint32_t CharacterOffset;

public:
    IndexWorker(
            IndexBundleConfiguration* bundleConfig,
            DirectoryRotator& directoryRotator,
            boost::shared_ptr<IndexManager> indexManager);

    ~IndexWorker();

public:
    bool index(const unsigned int& numdoc, bool& ret);

    bool optimizeIndexIdSpace();

    bool buildCollection(unsigned int numdoc);

    bool rebuildCollection(unsigned int numdoc);

    bool optimizeIndex();

    bool createDocument(const ::izenelib::driver::Value& documentValue);

    bool updateDocument(const ::izenelib::driver::Value& documentValue);

    bool destroyDocument(const ::izenelib::driver::Value& documentValue);

    bool getIndexStatus(Status& status);

    uint32_t getDocNum();

    uint32_t getKeyCount(const std::string& property_name);

private:
    void createPropertyList_();

    void doMining_();

    bool completePartialDocument_(docid_t oldId, Document& doc);

    bool getPropertyValue_( const PropertyValue& value, std::string& valueStr );

    bool doBuildCollection_(
            const std::string& scdFile,
            int op,
            uint32_t numdoc
    );

    bool insertOrUpdateSCD_(
            ScdParser& parser,
            bool isInsert,
            uint32_t numdoc,
            time_t timestamp
    );

    bool createUpdateDocId_(
            const izenelib::util::UString& scdDocId,
            bool rType,
            docid_t& oldId,
            docid_t& newId
    );

    bool createInsertDocId_(
            const izenelib::util::UString& scdDocId,
            docid_t& newId
    );

    void logCreatedDocToLogServer(const SCDDoc& scdDoc);

    bool fetchSCDFromLogServer(const std::string& scdPath);

    bool deleteSCD_(ScdParser& parser, time_t timestamp);

    bool insertDoc_(Document& document, IndexerDocument& indexDocument, time_t timestamp);

    bool updateDoc_(Document& document, IndexerDocument& indexDocument, time_t timestamp, bool rType);

    bool deleteDoc_(docid_t docid, time_t timestamp);

    void savePriceHistory_(int op);

    void saveSourceCount_(int op);

    bool prepareDocument_(
            SCDDoc& doc,
            Document& document,
            docid_t& oldId,
            bool& rType,
            std::map<std::string, pair<PropertyDataType, izenelib::util::UString> >& rTypeFieldValue,
            std::string& source,
            time_t& timestamp,
            bool insert = true
    );

    bool prepareIndexDocument_(docid_t oldId, const Document& document, IndexerDocument& indexDocument);

    bool checkSeparatorType_(
            const izenelib::util::UString& propertyValueStr,
            izenelib::util::UString::EncodingType encoding,
            char separator
    );

    bool preparePartialDocument_(
            Document& document,
            IndexerDocument& oldIndexDocument
    );

    bool checkRtype_(
            SCDDoc& doc,
            std::map<std::string, pair<PropertyDataType, izenelib::util::UString> >& rTypeFieldValue,
            bool& isUpdate
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

    size_t getTotalScdSize_();

    bool requireBackup_(size_t currTotalScdSize);

    bool backup_();

    bool recoverSCD_();

    static void value2SCDDoc(
            const ::izenelib::driver::Value& value,
            SCDDoc& scddoc
    );

private:
    IndexBundleConfiguration* bundleConfig_;
    MiningTaskService* miningTaskService_;
    RecommendTaskService* recommendTaskService_;

    boost::shared_ptr<LAManager> laManager_;
    boost::shared_ptr<IDManager> idManager_;
    boost::shared_ptr<DocumentManager> documentManager_;
    boost::shared_ptr<IndexManager> indexManager_;
    boost::shared_ptr<SearchWorker> searchWorker_;

    DirectoryRotator& directoryRotator_;
    PropertyConfig dateProperty_;
    config_tool::PROPERTY_ALIAS_MAP_T propertyAliasMap_;

    ScdWriterController* scd_writer_;
    TextSummarizationSubManager summarizer_;

    unsigned int collectionId_;
    IndexingProgress indexProgress_;
    bool checkInsert_;
    unsigned int numDeletedDocs_;
    unsigned int numUpdatedDocs_;

    Status indexStatus_;

    std::vector<boost::shared_ptr<LAInput> > laInputs_;
    std::vector<string> propertyList_;
    std::map<std::string, uint32_t> productSourceCount_;
    boost::shared_ptr<IndexHooker> hooker_;

    size_t totalSCDSizeSinceLastBackup_;

    friend class WorkerServer;
    friend class IndexSearchService;
    friend class IndexBundleActivator;
    friend class ProductBundleActivator;
};

}

#endif /* INDEX_WORKER_H_ */
