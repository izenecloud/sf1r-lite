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
#include <document-manager/Document.h>
#include <document-manager/text-summarization-submanager/TextSummarizationSubManager.h>
#include <common/Status.h>
#include <common/IndexingProgress.h>
#include <common/ScdParser.h>
#include <common/ScdWriterController.h>
#include <index-manager/IncSupportedIndexManager.h>

#include <ir/id_manager/IDManager.h>
#include <ir/index_manager/index/IndexerDocument.h>
#include <util/cronexpression.h>

#include <util/driver/Value.h>
#include <3rdparty/am/stx/btree_map.h>

#include <boost/shared_ptr.hpp>
#include <boost/tuple/tuple.hpp>
#include <util/concurrent_queue.h>
//#include <boost/atomic.hpp>

namespace sf1r
{

using izenelib::ir::idmanager::IDManager;

class IndexBundleConfiguration;
class MiningTaskService;
class RecommendTaskService;
class DocumentManager;
class LAManager;
class SearchManager;
class MiningManager;
class ScdWriterController;
class SearchWorker;
class DistributeRequestHooker;
class IndexHooker;

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

    // updatetype, newDoc, oldDocId, timestamp, OldDocumentForRType
    typedef boost::tuple<UpdateType, Document, docid_t, time_t, Document> UpdateBufferDataType;
    typedef stx::btree_map<docid_t, UpdateBufferDataType> UpdateBufferType;

public:
    IndexWorker(
            IndexBundleConfiguration* bundleConfig,
            DirectoryRotator& directoryRotator);

    ~IndexWorker();

    void setManager_test(boost::shared_ptr<IDManager> idManager,
        boost::shared_ptr<DocumentManager> documentManager)
    {
        idManager_ = idManager;
        documentManager_ = documentManager;
    }

public:
    virtual bool bindCallProxy(CallProxyType& proxy)
    {
        BIND_CALL_PROXY_BEGIN(IndexWorker, proxy)
        BIND_CALL_PROXY_3(index, std::string, unsigned int, bool)
        BIND_CALL_PROXY_3(HookDistributeRequestForIndex, int, std::string, bool)
        BIND_CALL_PROXY_END()
    }

    void HookDistributeRequestForIndex(int hooktype, const std::string& reqdata, bool& result);

    void index(const std::string& scd_path, unsigned int numdoc, bool& result);

    bool reindex(boost::shared_ptr<DocumentManager>& documentManager, int64_t timestamp);

    bool buildCollection(const std::string& scd_path, unsigned int numdoc);
    bool buildCollectionOnReplica(unsigned int numdoc);
    bool buildCollection(unsigned int numdoc, const std::vector<std::string>& scdList, int64_t timestamp);

    bool rebuildCollection(boost::shared_ptr<DocumentManager>& documentManager, int64_t timestamp);

    bool optimizeIndex();

    bool createDocument(const ::izenelib::driver::Value& documentValue);

    bool updateDocument(const ::izenelib::driver::Value& documentValue);
    bool updateDocumentInplace(const ::izenelib::driver::Value& request);

    bool destroyDocument(const ::izenelib::driver::Value& documentValue);

    bool getIndexStatus(Status& status);

    boost::shared_ptr<DocumentManager> getDocumentManager() const;

    void flush(bool mergeBarrel = false);

    IncSupportedIndexManager& getIncSupportedIndexManager()
    {
        return inc_supported_index_manager_;
    }

private:
    void createPropertyList_();

    void doMining_(int64_t timestamp);

    bool getPropertyValue_( const PropertyValue& value, std::string& valueStr );

    bool doBuildCollection_(const std::string& scdFile, SCD_TYPE scdType, uint32_t numdoc);

    bool insertOrUpdateSCD_(
            ScdParser& parser,
            SCD_TYPE scdType,
            uint32_t numdoc,
            time_t timestamp);

    bool createInsertDocId_(const uint128_t& scdDocId, docid_t& newId);

    bool deleteSCD_(ScdParser& parser, time_t timestamp);

    bool insertDoc_(
            size_t wid,
            Document& document,
            time_t timestamp,
            bool immediately = false);

    bool doInsertDoc_(
            Document& document,
            time_t timestamp);

    bool updateDoc_(
            size_t wid,
            docid_t oldId,
            Document& document,
            const Document& old_rtype_doc,
            time_t timestamp,
            IndexWorker::UpdateType updateType,
            bool immediately = false);

    bool doUpdateDoc_(
            docid_t oldId,
            Document& document,
            const Document& old_rtype_doc,
            IndexWorker::UpdateType updateType,
            time_t timestamp);

    void flushUpdateBuffer_(size_t wid);

    bool deleteDoc_(docid_t docid, time_t timestamp);

    void savePriceHistory_(int op);

    void saveSourceCount_(SCD_TYPE scdType);

    bool prepareDocIdAndUpdateType_(const izenelib::util::UString& scdDocIdUStr,
        const SCDDoc& scddoc, SCD_TYPE scdType,
        docid_t& oldDocId, docid_t& newDocId, IndexWorker::UpdateType& updateType);

    bool prepareDocIdAndUpdateType_(const std::string& scdDocIdStr,
        const SCDDoc& scddoc, SCD_TYPE scdType,
        docid_t& oldDocId, docid_t& newDocId, IndexWorker::UpdateType& updateType);

    bool prepareDocIdAndUpdateType_(const uint128_t& scdDocId,
        const SCDDoc& scddoc, SCD_TYPE scdType,
        docid_t& oldDocId, docid_t& newDocId, IndexWorker::UpdateType& updateType);

    bool prepareDocument_(
            const SCDDoc& doc,
            Document& document,
            Document& old_rtype_doc,
            const docid_t& oldId,
            const docid_t& docId,
            std::string& source,
            time_t& timestamp,
            const UpdateType& updateType,
            SCD_TYPE scdType);

    bool mergeDocument_(
            docid_t oldId,
            Document& doc);

    UpdateType checkUpdateType_(
            const uint128_t& scdDocId,
            const SCDDoc& doc,
            docid_t& oldId,
            docid_t& docId,
            SCD_TYPE scdType);

    UpdateType getUpdateType_(
            const uint128_t& scdDocId,
            const SCDDoc& doc,
            docid_t& oldId,
            SCD_TYPE scdType) const;


    bool makeSentenceBlocks_(
            const PropertyValueType& text,
            const unsigned int numOfSummary,
            const unsigned int maxDisplayLength,
            std::vector<CharacterOffset>& sentenceOffsetList);

    size_t getTotalScdSize_(const std::vector<std::string>& scdlist);

    static void value2SCDDoc(const ::izenelib::driver::Value& value, SCDDoc& scddoc);

    static void document2SCDDoc(const Document& document, SCDDoc& scddoc);

    /**
     * notify to clear cache on master.
     */
    void clearMasterCache_();

    void scheduleOptimizeTask();
    void lazyOptimizeIndex(int calltype);
    void indexSCDDocFunc(int workerid);

private:
    IndexBundleConfiguration* bundleConfig_;
    MiningTaskService* miningTaskService_;
    RecommendTaskService* recommendTaskService_;

    IncSupportedIndexManager inc_supported_index_manager_;

    boost::shared_ptr<IDManager> idManager_;
    boost::shared_ptr<DocumentManager> documentManager_;
    //boost::shared_ptr<IndexManager> indexManager_;
    boost::shared_ptr<SearchWorker> searchWorker_;

    DirectoryRotator& directoryRotator_;
    PropertyConfig dateProperty_;

    ScdWriterController* scd_writer_;
    TextSummarizationSubManager summarizer_;

    IndexingProgress indexProgress_;
    bool checkInsert_;
    unsigned int numDeletedDocs_;
    unsigned int numUpdatedDocs_;

    Status indexStatus_;

    //std::vector<boost::shared_ptr<LAInput> > laInputs_;
    std::vector<string> propertyList_;
    std::map<std::string, uint32_t> productSourceCount_;

    size_t totalSCDSizeSinceLastBackup_;

    boost::shared_ptr<IndexHooker> hooker_;
    std::vector<UpdateBufferType> updateBuffer_;
    DistributeRequestHooker *distribute_req_hooker_;

    izenelib::util::CronExpression scheduleExpression_;
    std::string optimizeJobDesc_;

    friend class IndexSearchService;
    friend class IndexBundleActivator;
    friend class ProductBundleActivator;

    struct IndexDocInfo
    {
        SCDDocPtr docptr;
        docid_t oldDocId;
        docid_t newDocId;
        SCD_TYPE scdType;
        UpdateType updateType;
        time_t timestamp;
        IndexDocInfo()
            : scdType(NOT_SCD), timestamp(0)
        {
        }
        IndexDocInfo(SCDDocPtr i_doc, docid_t i_oid, docid_t i_nid,
            const SCD_TYPE& i_scdType, const UpdateType& i_updatetype, const time_t& i_time)
            : docptr(i_doc), oldDocId(i_oid), newDocId(i_nid),
            scdType(i_scdType), updateType(i_updatetype), timestamp(i_time)
        {
        }
    };

    std::vector<izenelib::util::concurrent_queue<IndexDocInfo>* > asynchronousTasks_;
    std::vector<boost::thread*> index_thread_workers_;
    bool* index_thread_status_;
    bool is_real_time_;
};

}

#endif /* INDEX_WORKER_H_ */
