/**
 * @file IndexWorker.h
 * @author Zhongxia Li
 * @date Dec 5, 2011
 * @brief Index task worker
 */
#ifndef INDEX_WORKER_H_
#define INDEX_WORKER_H_

#include <net/aggregator/BindCallProxyBase.h>
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
#include <util/cronexpression.h>

#include <util/driver/Value.h>
#include <3rdparty/am/stx/btree_map.h>

#include <boost/shared_ptr.hpp>
#include <boost/tuple/tuple.hpp>

namespace sf1r
{

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
class DistributeRequestHooker;

class IndexWorker : public net::aggregator::BindCallProxyBase<IndexWorker>
{
    typedef uint32_t CharacterOffset;

    enum UpdateType
    {
        UNKNOWN,
        INSERT, ///Not update, it's a new document
        GENERAL, ///General update, equals to del + insert
        REPLACE, ///Don't need to change index, just adjust DocumentManager
        RTYPE  ///RType update to index
    };

    typedef boost::tuple<UpdateType, Document, IndexerDocument, IndexerDocument> UpdateBufferDataType;
    typedef stx::btree_map<docid_t, UpdateBufferDataType> UpdateBufferType;

public:
    IndexWorker(
            IndexBundleConfiguration* bundleConfig,
            DirectoryRotator& directoryRotator,
            boost::shared_ptr<IndexManager> indexManager);

    ~IndexWorker();

    void setManager_test(boost::shared_ptr<IDManager> idManager,
        boost::shared_ptr<DocumentManager> documentManager,
        boost::shared_ptr<IndexManager> indexManager,
        boost::shared_ptr<LAManager> laManager)
    {
        idManager_ = idManager;
        documentManager_ = documentManager;
        indexManager_ = indexManager;
        laManager_ = laManager;
    }

public:
    virtual bool bindCallProxy(CallProxyType& proxy)
    {
        BIND_CALL_PROXY_BEGIN(IndexWorker, proxy)
        BIND_CALL_PROXY_2(index, unsigned int, bool)
        BIND_CALL_PROXY_3(HookDistributeRequestForIndex, int, std::string, bool)
        BIND_CALL_PROXY_END()
    }

    void HookDistributeRequestForIndex(int hooktype, const std::string& reqdata, bool& result);

    void index(unsigned int numdoc, bool& result);

    bool reindex(boost::shared_ptr<DocumentManager>& documentManager);

    bool buildCollection(unsigned int numdoc);
    bool buildCollectionOnReplica(unsigned int numdoc);
    bool buildCollection(unsigned int numdoc, const std::vector<std::string>& scdList);

    bool rebuildCollection(boost::shared_ptr<DocumentManager>& documentManager);

    bool optimizeIndex();

    bool createDocument(const ::izenelib::driver::Value& documentValue);

    bool updateDocument(const ::izenelib::driver::Value& documentValue);
    bool updateDocumentInplace(const ::izenelib::driver::Value& request);

    bool destroyDocument(const ::izenelib::driver::Value& documentValue);

    bool getIndexStatus(Status& status);

    uint32_t getDocNum();

    uint32_t getKeyCount(const std::string& property_name);

    boost::shared_ptr<DocumentManager> getDocumentManager() const;

    void flush();
    bool reload();

private:
    void createPropertyList_();

    void doMining_();

    bool getPropertyValue_( const PropertyValue& value, std::string& valueStr );

    bool doBuildCollection_(const std::string& scdFile, int op, uint32_t numdoc);

    bool insertOrUpdateSCD_(
            ScdParser& parser,
            bool isInsert,
            uint32_t numdoc,
            time_t timestamp);

    bool createInsertDocId_(const uint128_t& scdDocId, docid_t& newId);

    bool deleteSCD_(ScdParser& parser, time_t timestamp);

    bool insertDoc_(
            Document& document,
            IndexerDocument& indexDocument,
            time_t timestamp,
            bool immediately = false);

    bool doInsertDoc_(
            Document& document,
            IndexerDocument& indexDocument);

    bool updateDoc_(
            Document& document,
            IndexerDocument& indexDocument,
            IndexerDocument& oldIndexDocument,
            time_t timestamp,
            IndexWorker::UpdateType updateType,
            bool immediately = false);

    bool doUpdateDoc_(
            Document& document,
            IndexerDocument& indexDocument,
            IndexerDocument& oldIndexDocument,
            IndexWorker::UpdateType updateType);

    void flushUpdateBuffer_();

    bool deleteDoc_(docid_t docid, time_t timestamp);

    void savePriceHistory_(int op);

    void saveSourceCount_(int op);

    bool prepareDocument_(
            SCDDoc& doc,
            Document& document,
            IndexerDocument& indexDocument,
            IndexerDocument& oldIndexDocument,
            docid_t& oldId,
            std::string& source,
            time_t& timestamp,
            UpdateType& updateType,
            bool insert = true);

    bool mergeDocument_(
            docid_t oldId,
            Document& doc,
            IndexerDocument& indexDocument,
            bool generateIndexDoc);

    bool prepareIndexDocument_(
            docid_t oldId,
            time_t timestamp,
            const Document& document,
            IndexerDocument& indexDocument);

    bool prepareIndexDocumentProperty_(
            docid_t docId,
            const FieldPair& p,
            IndexBundleSchema::iterator iter,
            IndexerDocument& indexDocument);

    bool prepareIndexDocumentStringProperty_(
            docid_t docId,
            const FieldPair& p,
            IndexBundleSchema::iterator iter,
            IndexerDocument& indexDocument);

    bool prepareIndexRTypeProperties_(
            docid_t docId,
            IndexerDocument& indexDocument);

    bool prepareIndexDocumentNumericProperty_(
            docid_t docId,
            const izenelib::util::UString & propertyValueU,
            IndexBundleSchema::iterator iter,
            IndexerDocument& indexDocument);

    bool checkSeparatorType_(
            const izenelib::util::UString& propertyValueStr,
            izenelib::util::UString::EncodingType encoding,
            char separator);

    UpdateType checkUpdateType_(
            const uint128_t& scdDocId,
            SCDDoc& doc,
            docid_t& oldId,
            docid_t& docId);

    bool makeSentenceBlocks_(
            const izenelib::util::UString& text,
            const unsigned int numOfSummary,
            const unsigned int maxDisplayLength,
            std::vector<CharacterOffset>& sentenceOffsetList);

    bool makeForwardIndex_(
            docid_t docId,
            const izenelib::util::UString& text,
            const std::string& propertyName,
            unsigned int propertyId,
            const AnalysisInfo& analysisInfo);

    size_t getTotalScdSize_(const std::vector<std::string>& scdlist);

    bool requireBackup_(size_t currTotalScdSize);

    bool backup_();

    bool recoverSCD_();

    static void value2SCDDoc(const ::izenelib::driver::Value& value, SCDDoc& scddoc);

    static void document2SCDDoc(const Document& document, SCDDoc& scddoc);

    /**
     * notify to clear cache on master.
     */
    void clearMasterCache_();

    void scheduleOptimizeTask();
    void lazyOptimizeIndex(int calltype);

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

    UpdateBufferType updateBuffer_;
    DistributeRequestHooker *distribute_req_hooker_;

    izenelib::util::CronExpression scheduleExpression_;
    std::string optimizeJobDesc_;

    friend class IndexSearchService;
    friend class IndexBundleActivator;
    friend class ProductBundleActivator;
};

}

#endif /* INDEX_WORKER_H_ */
