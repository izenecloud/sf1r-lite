#include "IndexWorker.h"
#include "SearchWorker.h"
#include "WorkerHelper.h"

#include <index-manager/IndexHooker.h>
#include <search-manager/SearchManager.h>
#include <mining-manager/MiningManager.h>
#include <common/NumericPropertyTable.h>
#include <common/NumericRangePropertyTable.h>
#include <common/RTypeStringPropTable.h>
#include <document-manager/DocumentManager.h>
#include <log-manager/ProductCount.h>
#include <log-manager/LogServerRequest.h>
#include <log-manager/LogServerConnection.h>
#include <common/JobScheduler.h>
#include <common/Utilities.h>
#include <aggregator-manager/MasterNotifier.h>
#include <node-manager/RequestLog.h>
#include <node-manager/DistributeFileSyncMgr.h>
#include <node-manager/DistributeRequestHooker.h>
#include <node-manager/NodeManagerBase.h>
#include <node-manager/MasterManagerBase.h>
#include <node-manager/DistributeFileSys.h>
#include <util/driver/Request.h>

// xxx
#include <bundles/index/IndexBundleConfiguration.h>
#include <bundles/mining/MiningTaskService.h>
#include <bundles/recommend/RecommendTaskService.h>
#include <node-manager/synchro/SynchroFactory.h>
#include <util/profiler/ProfilerGroup.h>
#include <util/scheduler.h>

#include <la/util/UStringUtil.h>

#include <glog/logging.h>

#include <boost/filesystem.hpp>

namespace bfs = boost::filesystem;

using izenelib::util::UString;
using namespace izenelib::driver;

namespace sf1r
{

namespace
{
/** the directory for scd file backup */
const char* SCD_BACKUP_DIR = "backup";
const std::string DOCID("DOCID");
const std::string DATE("DATE");
const size_t UPDATE_BUFFER_CAPACITY = 8192;
//PropertyConfig tempPropertyConfig;
const std::size_t INDEX_THREAD = 1;
const std::size_t INDEX_THREAD_QUEUE = UPDATE_BUFFER_CAPACITY*10;

}

IndexWorker::IndexWorker(
        IndexBundleConfiguration* bundleConfig,
        DirectoryRotator& directoryRotator)
    : bundleConfig_(bundleConfig)
    , miningTaskService_(NULL)
    , recommendTaskService_(NULL)
    , directoryRotator_(directoryRotator)
    , scd_writer_(new ScdWriterController(bundleConfig_->logSCDPath()))
    , indexProgress_()
    , checkInsert_(false)
    , numDeletedDocs_(0)
    , numUpdatedDocs_(0)
    , totalSCDSizeSinceLastBackup_(0)
    , distribute_req_hooker_(DistributeRequestHooker::get())
{
    bool hasDateInConfig = false;
    const IndexBundleSchema& indexSchema = bundleConfig_->indexSchema_;
    for (IndexBundleSchema::const_iterator iter = indexSchema.begin(), iterEnd = indexSchema.end();
            iter != iterEnd; ++iter)
    {
        std::string propertyName = iter->getName();
        boost::to_lower(propertyName);
        if (propertyName == "date")
        {
            hasDateInConfig = true;
            dateProperty_ = *iter;
            break;
        }
    }
    if (!hasDateInConfig)
        throw std::runtime_error("Date Property Doesn't exist in config");

    //indexStatus_.numDocs_ = inc_supported_index_manager_.numDocs();

    createPropertyList_();
    scd_writer_->SetFlushLimit(500);
    scheduleOptimizeTask();
    
    index_thread_status_ = new bool[INDEX_THREAD];
    asynchronousTasks_.resize(INDEX_THREAD);
    for(size_t i = 0; i < INDEX_THREAD; ++i)
    {
        asynchronousTasks_[i] = new izenelib::util::concurrent_queue<IndexDocInfo>(INDEX_THREAD_QUEUE);
        boost::thread* worker_thread = new boost::thread(boost::bind(&IndexWorker::indexSCDDocFunc, this, i));
        index_thread_workers_.push_back(worker_thread);
    }
    updateBuffer_.resize(INDEX_THREAD);
    is_real_time_ = false;
}

IndexWorker::~IndexWorker()
{
    delete scd_writer_;
    if (!optimizeJobDesc_.empty())
        izenelib::util::Scheduler::removeJob(optimizeJobDesc_);

    for(size_t i = 0; i < index_thread_workers_.size(); ++i)
    {
        index_thread_workers_[i]->interrupt();
        asynchronousTasks_[i]->push(IndexDocInfo());
        if (boost::this_thread::get_id() != index_thread_workers_[i]->get_id())
            index_thread_workers_[i]->join();
        delete index_thread_workers_[i];
        delete asynchronousTasks_[i];
    }
    delete[] index_thread_status_;
}

void IndexWorker::HookDistributeRequestForIndex(int hooktype, const std::string& reqdata, bool& result)
{
    LOG(INFO) << "new api request from shard, packeddata len: " << reqdata.size();
    MasterManagerBase::get()->pushWriteReq(reqdata, "api_from_shard");
    LOG(INFO) << "got hook request on the worker in indexworker." << reqdata;
    result = true;
}

void IndexWorker::flush(bool mergeBarrel)
{
    LOG(INFO) << "Begin flushing in IndexWorker ....";
    documentManager_->flush();
    idManager_->flush();
    LOG(INFO) << "Begin flushing inc_supported_index_manager_ in IndexWorker ....";
    inc_supported_index_manager_.flush();
    if (mergeBarrel)
    {
        LOG(INFO) << "Begin merge index ....";
        inc_supported_index_manager_.optimize(true);
        LOG(INFO) << "End merge index ....";
    }
    if (miningTaskService_)
        miningTaskService_->flush();
    LOG(INFO) << "Flushing finished in IndexWorker";
}

void IndexWorker::index(const std::string& scd_path, unsigned int numdoc, bool& result)
{
    result = true;
    if (distribute_req_hooker_->isRunningPrimary())
    {
        // in distributed, all write api need be called in sync.
        if (!distribute_req_hooker_->isHooked())
        {
            task_type task = boost::bind(&IndexWorker::buildCollection, this, scd_path, numdoc);
            JobScheduler::get()->addTask(task, bundleConfig_->collectionName_);
        }
        else
        {
            result = buildCollection(scd_path, numdoc);
        }
    }
    else
    {
        result = buildCollectionOnReplica(numdoc);
    }
}

bool IndexWorker::reindex(boost::shared_ptr<DocumentManager>& documentManager,
    int64_t timestamp)
{
    rebuildCollection(documentManager, timestamp);
    return true;
}

bool IndexWorker::buildCollection(const std::string& scd_path, unsigned int numdoc)
{
    DISTRIBUTE_WRITE_BEGIN;
    DISTRIBUTE_WRITE_CHECK_VALID_RETURN;
    ///If current directory is the one rotated from the backup directory,
    ///there should exist some missed SCDs since the last backup time,
    ///so we move those SCDs from backup directory, so that these data
    ///could be recovered through indexing

    string scdPath = bundleConfig_->indexSCDPath();
    if (!scd_path.empty())
        scdPath = scd_path;

    Status::Guard statusGuard(indexStatus_);

    ScdParser parser(bundleConfig_->encoding_);

    // saves the name of the scd files in the path
    std::vector<std::string> scdList;
    try
    {
        if (!bfs::is_directory(scdPath))
        {
            LOG(ERROR) << "SCD Path does not exist. Path " << scdPath;
            return false;
        }
    }
    catch (bfs::filesystem_error& e)
    {
        LOG(ERROR) << "Error while opening directory " << e.what();
        return false;
    }

    // search the directory for files
    static const bfs::directory_iterator kItrEnd;
    for (bfs::directory_iterator itr(scdPath); itr != kItrEnd; ++itr)
    {
        if (bfs::is_regular_file(itr->status()))
        {
            std::string fileName = itr->path().filename().string();
            if (parser.checkSCDFormat(fileName))
            {
                scdList.push_back(itr->path().string());
                parser.load(scdPath + fileName);
                indexProgress_.totalFileSize_ += parser.getFileSize();
            }
            else
            {
                LOG(WARNING) << "SCD File not valid " << fileName;
            }
        }
    }
    //sort scdList
    sort(scdList.begin(), scdList.end(), ScdParser::compareSCD);

    IndexReqLog reqlog;
    reqlog.scd_list = scdList;
    reqlog.timestamp = Utilities::createTimeStamp();
    if(!distribute_req_hooker_->prepare(Req_Index, dynamic_cast<CommonReqData&>(reqlog)))
    {
        LOG(ERROR) << "prepare failed in " << __FUNCTION__;
        return false;
    }

    bool ret = buildCollection(numdoc, scdList, reqlog.timestamp);

    DISTRIBUTE_WRITE_FINISH(ret);

    return ret;
}

bool IndexWorker::buildCollectionOnReplica(unsigned int numdoc)
{
    DISTRIBUTE_WRITE_BEGIN;
    DISTRIBUTE_WRITE_CHECK_VALID_RETURN;

    Status::Guard statusGuard(indexStatus_);

    IndexReqLog reqlog;
    if(!distribute_req_hooker_->prepare(Req_Index, reqlog))
    {
        LOG(ERROR) << "prepare failed in " << __FUNCTION__;
        return false;
    }

    LOG(INFO) << "got index from primary/log, scd list is : ";
    for (size_t i = 0; i < reqlog.scd_list.size(); ++i)
    {
        LOG(INFO) << reqlog.scd_list[i];
        bfs::path backup_scd = bfs::path(reqlog.scd_list[i]);
        backup_scd = backup_scd.parent_path()/bfs::path("backup")/backup_scd.filename();
        if (bfs::exists(reqlog.scd_list[i]))
        {
            LOG(INFO) << "found index scd file in index directory.";
        }
        else if (bfs::exists(backup_scd))
        {
            // try to find in backup
            LOG(INFO) << "found index scd file in backup, move to index";
            bfs::rename(backup_scd, reqlog.scd_list[i]);
        }
        else if (!DistributeFileSyncMgr::get()->getFileFromOther(reqlog.scd_list[i]))
        {
            if (!DistributeFileSyncMgr::get()->getFileFromOther(backup_scd.string()))
            {
                LOG(INFO) << "index scd file missing." << reqlog.scd_list[i];
                throw std::runtime_error("index scd file missing!");
            }
            else
            {
                LOG(INFO) << "get index scd file from other's backup, move to index";
                bfs::rename(backup_scd, reqlog.scd_list[i]);
            }
        }
    }

    bool ret = buildCollection(numdoc, reqlog.scd_list, reqlog.timestamp);

    DISTRIBUTE_WRITE_FINISH(ret);

    return ret;
}

bool IndexWorker::buildCollection(unsigned int numdoc, const std::vector<std::string>& scdList, int64_t timestamp)
{
    CREATE_PROFILER(buildIndex, "Index:SIAProcess", "Indexer : buildIndex")

    START_PROFILER(buildIndex);
    LOG(INFO) << "start BuildCollection";
    izenelib::util::ClockTimer timer;

    //flush all writing SCDs
    scd_writer_->Flush();
    indexProgress_.reset();

    documentManager_->last_delete_docid_.clear();
    indexProgress_.totalFileSize_ = getTotalScdSize_(scdList);
    documentManager_->last_delete_docid_.clear();
    indexProgress_.totalFileNum = scdList.size();

    if (indexProgress_.totalFileNum == 0)
    {
        LOG(WARNING) << "SCD Files do not exist. Path ";
        if (miningTaskService_)
        {
            miningTaskService_->DoContinue();
        }
        return true;
    }

    indexProgress_.currentFileIdx = 1;

    inc_supported_index_manager_.preBuildFromSCD(indexProgress_.totalFileSize_);

    is_real_time_ = inc_supported_index_manager_.isRealTime();

    {
        DirectoryGuard dirGuard(directoryRotator_.currentDirectory().get());
        if (!dirGuard)
        {
            LOG(ERROR) << "Index directory is corrupted";
            return false;
        }

        LOG(INFO) << "SCD Files in Path processed in given order.";
        vector<string>::const_iterator scd_it;
        for (scd_it = scdList.begin(); scd_it != scdList.end(); ++scd_it)
            LOG(INFO) << "SCD File " << bfs::path(*scd_it).stem();

        try
        {
            ScdParser parser(bundleConfig_->encoding_);
            // loops the list of SCD files that belongs to this collection
            long proccessedFileSize = 0;
            for (scd_it = scdList.begin(); scd_it != scdList.end(); scd_it++)
            {
                size_t pos = scd_it ->rfind("/") + 1;
                string filename = scd_it ->substr(pos);
                indexProgress_.currentFileName = filename;
                indexProgress_.currentFilePos_ = 0;

                LOG(INFO) << "Processing SCD file. " << bfs::path(*scd_it).stem();

                SCD_TYPE scdType = parser.checkSCDType(*scd_it);

                if (scdType != INSERT_SCD)
                {
                    numdoc = 0;
                }

                doBuildCollection_(*scd_it, scdType, numdoc);
                LOG(INFO) << "Indexing Finished";

                parser.load(*scd_it);
                proccessedFileSize += parser.getFileSize();
                indexProgress_.totalFilePos_ = proccessedFileSize;
                indexProgress_.getIndexingStatus(indexStatus_);
                ++indexProgress_.currentFileIdx;
                uint32_t scd_index = indexProgress_.currentFileIdx;
                if (scd_index % 50 == 0)
                {
                    std::string report_file_name = "PerformanceIndexResult.SIAProcess-" + boost::lexical_cast<std::string>(scd_index);
                    REPORT_PROFILE_TO_FILE(report_file_name.c_str())
                }

            } // end of loop for scd files of a collection

            documentManager_->flush();
            idManager_->flush();

            inc_supported_index_manager_.postBuildFromSCD(timestamp);

        }
        catch (std::exception& e)
        {
            LOG(WARNING) << "exception in indexing : " << e.what();
            indexProgress_.getIndexingStatus(indexStatus_);
            indexProgress_.reset();
            return false;
        }

        const boost::shared_ptr<Directory>& currentDir = directoryRotator_.currentDirectory();

        if (!DistributeFileSys::get()->isEnabled())
        {
            for (scd_it = scdList.begin(); scd_it != scdList.end(); ++scd_it)
            {
                bfs::path bkDir = bfs::path(*scd_it).parent_path() / SCD_BACKUP_DIR;
                LOG(INFO) << " SCD file : " << *scd_it << " backuped to " << bkDir;
                try
                {
                    bfs::create_directories(bkDir);
                    bfs::rename(*scd_it, bkDir / bfs::path(*scd_it).filename());
                    currentDir->appendSCD(bfs::path(*scd_it).filename().string());
                }
                catch (bfs::filesystem_error& e)
                {
                    LOG(WARNING) << "exception in rename file " << *scd_it << ": " << e.what();
                }
            }
        }

    }///set cookie as true here

    //@brief : to check if there is fuzzy numberic or date filter;
    std::set<std::string> suffix_numeric_filter_properties;
    std::vector<NumericFilterConfig>::iterator pit
        = miningTaskService_->getMiningBundleConfig()->mining_schema_.suffixmatch_schema.num_filter_properties.begin();

    while (pit != miningTaskService_->getMiningBundleConfig()->mining_schema_.suffixmatch_schema.num_filter_properties.end())
    {
        suffix_numeric_filter_properties.insert(pit->property);
        ++pit;
    }
    std::set<std::string> intersection;
    std::set_intersection(documentManager_->RtypeDocidPros_.begin(),
                          documentManager_->RtypeDocidPros_.end(),
                          suffix_numeric_filter_properties.begin(),
                          suffix_numeric_filter_properties.end(),
                          std::inserter(intersection, intersection.begin()));
    documentManager_->RtypeDocidPros_.swap(intersection);

    try
    {
        if (hooker_)
        {
            if (!hooker_->FinishHook(timestamp))
            {
                LOG(ERROR) << "[IndexWorker] Hooker Finish failed.";
                return false;
            }
            LOG(INFO) <<"[IndexWorker] Hooker Finished.";
        }

        if (miningTaskService_)
        {
            inc_supported_index_manager_.preMining();
            miningTaskService_->DoMiningCollection(timestamp);
            inc_supported_index_manager_.postMining();
        }
    }
    catch (std::exception& e)
    {
        LOG(WARNING) << "exception in mining: " << e.what();
        indexProgress_.getIndexingStatus(indexStatus_);
        indexProgress_.reset();
        return false;
    }

    inc_supported_index_manager_.finishIndex();

    indexProgress_.getIndexingStatus(indexStatus_);
    LOG(INFO) << "Indexing Finished! Documents Indexed: " << documentManager_->getMaxDocId()
              << " Deleted: " << numDeletedDocs_
              << " Updated: " << numUpdatedDocs_;

    //both variables are refreshed
    numDeletedDocs_ = 0;
    numUpdatedDocs_ = 0;

    indexProgress_.reset();

    STOP_PROFILER(buildIndex);

    REPORT_PROFILE_TO_FILE("PerformanceIndexResult.SIAProcess")
    LOG(INFO) << "End BuildCollection: ";
    LOG(INFO) << "time elapsed:" << timer.elapsed() <<"seconds";

    searchWorker_->clearSearchCache();
    searchWorker_->clearFilterCache();

    return true;
}

bool IndexWorker::rebuildCollection(boost::shared_ptr<DocumentManager>& documentManager,
    int64_t timestamp)
{
    LOG(INFO) << "start BuildCollection";

    if (!distribute_req_hooker_->isValid())
    {
        LOG(ERROR) << __FUNCTION__ << " call invalid.";
        return false;
    }

    if (!documentManager)
    {
        LOG(ERROR) << "documentManager is not initialized!";
        return false;
    }

    izenelib::util::ClockTimer timer;

    indexProgress_.reset();

    docid_t minDocId = 1;
    docid_t maxDocId = documentManager->getMaxDocId();
    docid_t curDocId = 0;
    docid_t insertedCount = 0;

    LOG(INFO) << "before rebuild : orig doc maxDocId is " << documentManager_->getMaxDocId();
    LOG(INFO) << "before rebuild : rebuild doc maxDocId is " << documentManager->getMaxDocId();
    for (curDocId = minDocId; curDocId <= maxDocId; curDocId++)
    {
        if (documentManager->isDeleted(curDocId))
        {
            //LOG(INFO) << "skip deleted docid: " << curDocId;
            continue;
        }

        Document document;
        bool b = documentManager->getDocument(curDocId, document);
        if (!b) continue;
        documentManager->getRTypePropertiesForDocument(curDocId, document);
        // update docid
        std::string docidName("DOCID");
        Document::doc_prop_value_strtype docidValueU;
        if (!document.getProperty(docidName, docidValueU))
        {
            //LOG(WARNING) << "skip doc which has no DOCID property: " << curDocId;
            continue;
        }

        docid_t newDocId;
        std::string docid_str = propstr_to_str(docidValueU);
        if (createInsertDocId_(Utilities::md5ToUint128(docid_str), newDocId))
        {
            //LOG(INFO) << document.getId() << " -> " << newDocId;
            document.setId(newDocId);
        }
        else
        {
            LOG(INFO) << "Failed to create new docid for: " << curDocId;
            continue;
        }

        documentManager_->copyRTypeValues(documentManager, curDocId, newDocId);
        if (!insertDoc_(0, document, timestamp, true))
            continue;

        insertedCount++;
        if (insertedCount % 10000 == 0)
        {
            LOG(INFO) << "inserted doc number: " << insertedCount;
        }

        // interrupt when closing the process
        boost::this_thread::interruption_point();
    }

    //flushUpdateBuffer_(0);

    LOG(INFO) << "inserted doc number: " << insertedCount << ", total: " << maxDocId;
    LOG(INFO) << "Indexing Finished";

    documentManager_->flush();
    idManager_->flush();

    inc_supported_index_manager_.finishRebuild();

    if (miningTaskService_)
    {
        inc_supported_index_manager_.preMining();
        miningTaskService_->DoMiningCollection(timestamp);
        inc_supported_index_manager_.postMining();
        miningTaskService_->flush();
    }

    LOG(INFO) << "End BuildCollection: ";
    LOG(INFO) << "time elapsed:" << timer.elapsed() <<"seconds";

    return true;
}

bool IndexWorker::optimizeIndex()
{
    DISTRIBUTE_WRITE_BEGIN;
    DISTRIBUTE_WRITE_CHECK_VALID_RETURN;

    NoAdditionNeedBackupReqLog reqlog;
    if(!distribute_req_hooker_->prepare(Req_NoAdditionData_NeedBackup_Req, reqlog))
    {
        LOG(ERROR) << "prepare failed in " << __FUNCTION__;
        return false;
    }

    DirectoryGuard dirGuard(directoryRotator_.currentDirectory().get());
    if (!dirGuard)
    {
        LOG(ERROR) << "Index directory is corrupted";
        return false;
    }

    inc_supported_index_manager_.optimize(false);

    //flush();

    DISTRIBUTE_WRITE_FINISH(true);
    return true;
}

void IndexWorker::scheduleOptimizeTask()
{
    if(!bundleConfig_->indexConfig_.indexStrategy_.optimizeSchedule_.empty())
    {
        scheduleExpression_.setExpression(bundleConfig_->indexConfig_.indexStrategy_.optimizeSchedule_);
        using namespace izenelib::util;
        int32_t uuid =  (int32_t)HashFunction<std::string>::generateHash32(bundleConfig_->indexConfig_.indexStrategy_.indexLocation_);
        char uuidstr[10];
        memset(uuidstr,0,10);
        sprintf(uuidstr,"%d",uuid);
        const string optimizeJob = "optimizeindex" + std::string(uuidstr);
        ///we need an uuid here because scheduler is an singleton in the system
        izenelib::util::Scheduler::removeJob(optimizeJob);
        optimizeJobDesc_ = optimizeJob;
        boost::function<void (int)> task = boost::bind(&IndexWorker::lazyOptimizeIndex, this, _1);
        izenelib::util::Scheduler::addJob(optimizeJob, 60*1000, 0, task);
        LOG(INFO) << "optimizeIndex schedule : " << optimizeJob;
    }
}

void IndexWorker::lazyOptimizeIndex(int calltype)
{
    if (scheduleExpression_.matches_now() || calltype > 0)
    {
        if (calltype == 0 && NodeManagerBase::get()->isDistributed())
        {
            if (NodeManagerBase::get()->isPrimary())
            {
                MasterManagerBase::get()->pushWriteReq(optimizeJobDesc_, "cron");
                LOG(INFO) << "push cron job to queue on primary : " << optimizeJobDesc_;
            }
            else
            {
                LOG(INFO) << "cron job on replica ignored. ";
            }
            return;
        }
        
        DISTRIBUTE_WRITE_BEGIN;
        DISTRIBUTE_WRITE_CHECK_VALID_RETURN2;

        CronJobReqLog reqlog;
        reqlog.cron_time = Utilities::createTimeStamp();
        if (!DistributeRequestHooker::get()->prepare(Req_CronJob, reqlog))
        {
            LOG(ERROR) << "!!!! prepare log failed while running cron job. : " << optimizeJobDesc_ << std::endl;
            return;
        }

        LOG(INFO) << "optimizeIndex cron job begin running: " << optimizeJobDesc_;
        inc_supported_index_manager_.optimize(false);

        DISTRIBUTE_WRITE_FINISH(true);
    }
}

void IndexWorker::doMining_(int64_t timestamp)
{
    if (miningTaskService_)
    {
        std::string cronStr = miningTaskService_->getMiningBundleConfig()->mining_config_.dcmin_param.cron;
        if (cronStr.empty())
        {
            int docLimit = miningTaskService_->getMiningBundleConfig()->mining_config_.dcmin_param.docnum_limit;
            if (docLimit != 0 && (documentManager_->getMaxDocId() % docLimit) == 0)
            {
                miningTaskService_->DoMiningCollection(timestamp);
            }
        }
    }
}

bool IndexWorker::createDocument(const Value& documentValue)
{
    DISTRIBUTE_WRITE_BEGIN;
    DISTRIBUTE_WRITE_CHECK_VALID_RETURN;

    docid_t docid;
    uint128_t num_docid = Utilities::md5ToUint128(asString(documentValue["DOCID"]));
    if (idManager_->getDocIdByDocName(num_docid, docid, false))
    {
        if (!documentManager_->isDeleted(docid))
        {
            LOG(INFO) << "the document already exist for : " << docid;
            return false;
        }
    }

    CreateOrUpdateDocReqLog reqlog;
    reqlog.timestamp = Utilities::createTimeStamp();
    if(!distribute_req_hooker_->prepare(Req_CreateOrUpdate_Doc, dynamic_cast<CommonReqData&>(reqlog)))
    {
        LOG(ERROR) << "prepare failed in " << __FUNCTION__;
        return false;
    }

    DirectoryGuard dirGuard(directoryRotator_.currentDirectory().get());
    if (!dirGuard)
    {
        LOG(ERROR) << "Index directory is corrupted";
        return false;
    }

    LOG(INFO) << "create doc timestamp is : " << reqlog.timestamp;

    SCDDoc scddoc;
    value2SCDDoc(documentValue, scddoc);
    scd_writer_->Write(scddoc, INSERT_SCD);

    Document document;
    Document old_rtype_doc;
    //IndexerDocument indexDocument, oldIndexDocument;
    docid_t oldId = 0;
    std::string source = "";
    IndexWorker::UpdateType updateType = INSERT;
    time_t timestamp = reqlog.timestamp;
    if (prepareDocIdAndUpdateType_(asString(documentValue["DOCID"]), scddoc, INSERT_SCD,
            oldId, docid, updateType))
    {
        return false;
    }
    if (!prepareDocument_(scddoc, document, old_rtype_doc, oldId, docid, source, timestamp, updateType, INSERT_SCD))
        return false;

    inc_supported_index_manager_.preProcessForAPI();

    bool ret = insertDoc_(0, document, reqlog.timestamp, true);
    if (ret)
    {
        doMining_(reqlog.timestamp);
    }
    //flush();

    DISTRIBUTE_WRITE_FINISH2(ret, reqlog);
    return ret;
}

IndexWorker::UpdateType IndexWorker::getUpdateType_(
        const uint128_t& scdDocId,
        const SCDDoc& doc,
        docid_t& oldId,
        SCD_TYPE scdType) const
{
    if (!idManager_->getDocIdByDocName(scdDocId, oldId, false))
    {
        return INSERT;
    }

    if (scdType == RTYPE_SCD)
    {
        return RTYPE;
    }

    PropertyConfig tempPropertyConfig;
    SCDDoc::const_iterator p = doc.begin();
    for (; p != doc.end(); ++p)
    {
        const string& fieldName = p->first;
        const ScdPropertyValueType& propertyValueU = p->second;
        if (boost::iequals(fieldName, DOCID))
            continue;
        if (propertyValueU.empty())
            continue;

        tempPropertyConfig.propertyName_ = fieldName;
        IndexBundleSchema::const_iterator iter = bundleConfig_->indexSchema_.find(tempPropertyConfig);

        if (iter == bundleConfig_->indexSchema_.end())
            continue;

        if( iter->isRTypeString() )
            continue;

        if (iter->isIndex())
        {
            if (iter->isAnalyzed())
            {
                return GENERAL;
            }
            else if (iter->getIsFilter() && iter->getType() != STRING_PROPERTY_TYPE)
            {
                continue;
            }
        }
        return GENERAL;
    }

    return RTYPE;
}


bool IndexWorker::updateDocument(const Value& documentValue)
{
    DISTRIBUTE_WRITE_BEGIN;
    DISTRIBUTE_WRITE_CHECK_VALID_RETURN;

    SCDDoc scddoc;
    value2SCDDoc(documentValue, scddoc);
    // check for if we can success.
    {
        docid_t olddocid;
        uint128_t num_docid = Utilities::md5ToUint128(asString(documentValue["DOCID"]));
        UpdateType updateType = getUpdateType_(num_docid, scddoc, olddocid, UPDATE_SCD);
        if (updateType == RTYPE)
        {
            if (documentManager_->isDeleted(olddocid))
            {
                LOG(INFO) << "update document for rtype failed for deleted doc." << olddocid;
                return false;
            }
        }
    }

    CreateOrUpdateDocReqLog reqlog;
    reqlog.timestamp = Utilities::createTimeStamp();
    if(!distribute_req_hooker_->prepare(Req_CreateOrUpdate_Doc, dynamic_cast<CommonReqData&>(reqlog)))
    {
        LOG(ERROR) << "prepare failed in: " << __FUNCTION__;
        return false;
    }

    DirectoryGuard dirGuard(directoryRotator_.currentDirectory().get());
    if (!dirGuard)
    {
        LOG(ERROR) << "Index directory is corrupted";
        return false;
    }


    time_t timestamp = reqlog.timestamp;
    LOG(INFO) << "update doc timestamp is : " << timestamp;

    Document document;
    Document old_rtype_doc;
    //IndexerDocument indexDocument, oldIndexDocument;
    docid_t oldId = 0;
    docid_t docid = 0;
    IndexWorker::UpdateType updateType = UNKNOWN;
    std::string source = "";

    if (prepareDocIdAndUpdateType_(asString(documentValue["DOCID"]), scddoc, UPDATE_SCD,
            oldId, docid, updateType))
    {
        return false;
    }
    if (!prepareDocument_(scddoc, document, old_rtype_doc, oldId, docid, source, timestamp, updateType, UPDATE_SCD))
    {
        return false;
    }

    inc_supported_index_manager_.preProcessForAPI();

    bool ret = updateDoc_(0, oldId, document, old_rtype_doc, reqlog.timestamp, updateType, true);
    if (ret && (updateType != IndexWorker::RTYPE))
    {
        if (!bundleConfig_->enable_forceget_doc_)
        {
            searchWorker_->clearSearchCache();
            ///clear filter cache because of * queries:
            ///filter will be added into documentiterator
            ///together with AllDocumentIterator
            searchWorker_->clearFilterCache();
        }
        doMining_(reqlog.timestamp);
    }

    if (updateType != IndexWorker::RTYPE)
    {
        documentManager_->getRTypePropertiesForDocument(document.getId(),document);
        document2SCDDoc(document,scddoc);
    }
    scd_writer_->Write(scddoc, UPDATE_SCD);

    //flush();
    DISTRIBUTE_WRITE_FINISH2(ret, reqlog);
    return ret;
}

bool IndexWorker::updateDocumentInplace(const Value& request)
{
    DISTRIBUTE_WRITE_BEGIN;
    DISTRIBUTE_WRITE_CHECK_VALID_RETURN;

    docid_t docid;
    uint128_t num_docid = Utilities::md5ToUint128(asString(request[DOCID]));
    if (!idManager_->getDocIdByDocName(num_docid, docid, false))
    {
        LOG(WARNING) << "updating in place error, document not exist, DOCID: " << asString(request[DOCID]) << std::endl;
        return false;
    }
    // get the update property detail
    Value update_detail = request["update"];
    Value::ArrayType* update_propertys = update_detail.getPtr<Value::ArrayType>();
    if (update_propertys == NULL || update_propertys->size() == 0)
    {
        LOG(WARNING) << "updating in place error, detail property empty." << std::endl;
        return false;
    }

    Value new_document;
    new_document[DOCID] = asString(request[DOCID]);

    PropertyConfig tempPropertyConfig;
    typedef Value::ArrayType::iterator iterator;
    for (iterator it = update_propertys->begin(); it != update_propertys->end(); ++it)
    {
        if (!it->hasKey("property") || !it->hasKey("op") || !it->hasKey("opvalue"))
            return false;

        std::string propname = asString((*it)["property"]);
        std::string op = asString((*it)["op"]);
        std::string opvalue = asString((*it)["opvalue"]);
        PropertyValue oldvalue;
        // get old property first, if old property not exist, inplace update will fail.
        if (documentManager_->getPropertyValue(docid, propname, oldvalue))
        {
            tempPropertyConfig.propertyName_ = propname;
            IndexBundleSchema::const_iterator iter = bundleConfig_->indexSchema_.find(tempPropertyConfig);
            //DocumentSchema::const_iterator iter = bundleConfig_->documentSchema_.find(tempPropertyConfig);

            //Numeric property should not be range type
            bool isValidProperty = (iter != bundleConfig_->indexSchema_.end() && !iter->bRange_);
            int inplace_type = 0;
            // determine how to do the inplace operation by different property type.
            if (isValidProperty)
            {
                switch(iter->propertyType_)
                {
                case INT8_PROPERTY_TYPE:
                case INT16_PROPERTY_TYPE:
                case INT32_PROPERTY_TYPE:
                case INT64_PROPERTY_TYPE:
                case DATETIME_PROPERTY_TYPE:
                    {
                        inplace_type = 0;
                        break;
                    }
                case FLOAT_PROPERTY_TYPE:
                    {
                        inplace_type = 1;
                        break;
                    }
                case DOUBLE_PROPERTY_TYPE:
                    {
                        inplace_type = 2;
                        break;
                    }
                    break;
                default:
                    {
                        LOG(INFO) << "property type: " << iter->propertyType_ << " does not support the inplace update." << std::endl;
                        return false;
                    }
                }
            }
            else
            {
                LOG(INFO) << "property : " << propname << " does not support the inplace update." << std::endl;
                return false;
            }

            try
            {
                std::string new_propvalue;
                std::string oldvalue_str;
                PropertyValue::PropertyValueStrType& propertyValueU = oldvalue.getPropertyStrValue();
                oldvalue_str = propstr_to_str(propertyValueU);
                ///if a numeric property does not contain valid value, suppose it to be zero
                if (oldvalue_str.empty()) oldvalue_str = "0";
                if (op == "add")
                {
                    if (inplace_type == 0)
                    {
                        int64_t newv = boost::lexical_cast<int64_t>(oldvalue_str) + boost::lexical_cast<int64_t>(opvalue);
                        new_propvalue = boost::lexical_cast<std::string>(newv);
                    }
                    else if (inplace_type == 1)
                    {
                        new_propvalue = boost::lexical_cast<std::string>(boost::lexical_cast<float>(oldvalue_str) +
                                boost::lexical_cast<float>(opvalue));
                    }
                    else if (inplace_type == 2)
                    {
                        new_propvalue = boost::lexical_cast<std::string>(boost::lexical_cast<double>(oldvalue_str) + boost::lexical_cast<double>(opvalue));
                    }
                    else
                    {
                        assert(false);
                    }
                }
                else
                {
                    LOG(INFO) << "not supported update method inplace. method: "  << op << std::endl;
                    return false;
                }
                new_document[propname] = new_propvalue;
            }
            catch (boost::bad_lexical_cast& e)
            {
                LOG(WARNING) << "do inplace update failed. " << e.what() << endl;
                return false;
            }
        }
        else
        {
            LOG(INFO) << "update property not found in document, property: " << propname << std::endl;
            return false;
        }
    }

    DISTRIBUTE_WRITE_FINISH3;
    // do the common update.
    return updateDocument(new_document);
}

bool IndexWorker::destroyDocument(const Value& documentValue)
{
    DISTRIBUTE_WRITE_BEGIN;
    DISTRIBUTE_WRITE_CHECK_VALID_RETURN;

    docid_t docid;
    uint128_t num_docid = Utilities::md5ToUint128(asString(documentValue["DOCID"]));

    if (!idManager_->getDocIdByDocName(num_docid, docid, false))
    {
        return false;
    }

    if (documentManager_->isDeleted(docid))
    {
        LOG(INFO) << "doc has been deleted already: " << docid;
        return false;
    }

    TimestampReqLog reqlog;
    reqlog.timestamp = Utilities::createTimeStamp();
    if(!distribute_req_hooker_->prepare(Req_WithTimestamp, dynamic_cast<CommonReqData&>(reqlog)))
    {
        LOG(ERROR) << "prepare failed in: " << __FUNCTION__;
        return false;
    }

    DirectoryGuard dirGuard(directoryRotator_.currentDirectory().get());
    if (!dirGuard)
    {
        LOG(ERROR) << "Index directory is corrupted";
        return false;
    }

    SCDDoc scddoc;
    value2SCDDoc(documentValue, scddoc);
    scd_writer_->Write(scddoc, DELETE_SCD);
    bool ret = deleteDoc_(docid, reqlog.timestamp);
    if (ret)
    {
        if (!bundleConfig_->enable_forceget_doc_)
        {
            searchWorker_->clearSearchCache();
            ///clear filter cache because of * queries:
            ///filter will be added into documentiterator
            ///together with AllDocumentIterator
            searchWorker_->clearFilterCache();
        }
        doMining_(reqlog.timestamp);
    }

    // delete from log server
    if (bundleConfig_->logCreatedDoc_)
    {
        DeleteScdDocRequest deleteReq;
        try
        {
            deleteReq.param_.docid_ = Utilities::md5ToUint128(asString(documentValue["DOCID"]));
            deleteReq.param_.collection_ = bundleConfig_->collectionName_;
        }
        catch (const std::exception)
        {
        }

        LogServerConnection::instance().asynRequest(deleteReq);
        LogServerConnection::instance().flushRequests();
    }

    //flush();

    DISTRIBUTE_WRITE_FINISH(ret);

    return ret;
}

bool IndexWorker::getIndexStatus(Status& status)
{
    indexProgress_.getIndexingStatus(indexStatus_);
    status = indexStatus_;
    return true;
}

boost::shared_ptr<DocumentManager> IndexWorker::getDocumentManager() const
{
    return documentManager_;
}

/// private ////////////////////////////////////////////////////////////////////

void IndexWorker::createPropertyList_()
{
    for (DocumentSchema::const_iterator iter = bundleConfig_->documentSchema_.begin();
            iter != bundleConfig_->documentSchema_.end(); ++iter)
    {
        string propertyName = iter->propertyName_;
        boost::to_lower(propertyName);
        propertyList_.push_back(propertyName);
    }
}

bool IndexWorker::getPropertyValue_(const PropertyValue& value, std::string& valueStr)
{
    try
    {
        const PropertyValue::PropertyValueStrType& sourceFieldValue = value.getPropertyStrValue();
        if (sourceFieldValue.empty()) return false;
        valueStr = propstr_to_str(sourceFieldValue);
        return true;
    }
    catch (boost::bad_get& e)
    {
        LOG(WARNING) << "exception in get property value: " << e.what();
        return false;
    }
}

bool IndexWorker::doBuildCollection_(
        const std::string& fileName,
        SCD_TYPE scdType,
        uint32_t numdoc)
{
    ScdParser parser(bundleConfig_->encoding_);
    if (!parser.load(fileName))
    {
        LOG(ERROR) << "Could not Load Scd File. File " << fileName;
        return false;
    }

    indexProgress_.currentFileSize_ = parser.getFileSize();
    indexProgress_.currentFilePos_ = 0;
    productSourceCount_.clear();

    // Filename: B-00-YYYYMMDDhhmm-ssuuu-I-C.SCD
    // Timestamp: YYYYMMDDThhmmss, fff
    std::string baseName(basename(fileName.c_str()));
    std::stringstream ss;
    ss << baseName.substr(5, 8);
    ss << "T";
    ss << baseName.substr(13, 4);
    ss << baseName.substr(18, 2);
    ss << ",";
    ss << baseName.substr(20, 3);
    boost::posix_time::ptime pt;
    try
    {
        pt = boost::posix_time::from_iso_string(ss.str());
    }
    catch (const std::exception& ex)
    {}
    time_t timestamp = Utilities::createTimeStamp(pt);
    if (timestamp == -1)
    {
        LOG(WARNING) << "!!!! create time from scd fileName failed. " << ss.str();
        timestamp = Utilities::createTimeStamp();
    }

    if (scdType == DELETE_SCD)
    {
        if (!deleteSCD_(parser, timestamp))
            return false;
    }
    else
    {
        if (!insertOrUpdateSCD_(parser, scdType, numdoc, timestamp))
            return false;
    }
    searchWorker_->reset_all_property_cache();

    clearMasterCache_();

    saveSourceCount_(scdType);

    return true;
}

static void setIndexThreadStatus(bool* status)
{
    *status = true;
}

void IndexWorker::indexSCDDocFunc(int workerid)
{
    Document document;
    Document old_rtype_doc;

    try{

    while (true)
    {
        IndexDocInfo workerdata;
        index_thread_status_[workerid] = false;

        asynchronousTasks_[workerid]->pop(workerdata,
            boost::bind(&setIndexThreadStatus, index_thread_status_ + workerid));

        if (!workerdata.docptr && (workerdata.scdType == NOT_SCD))
        {
            flushUpdateBuffer_(workerid);
            index_thread_status_[workerid] = false;
            boost::this_thread::interruption_point();
            continue;
        }
        boost::this_thread::interruption_point();

        std::string source = "";
        time_t new_timestamp = workerdata.timestamp;
        document.clear();
        old_rtype_doc.clear();

        if (!prepareDocument_(*(workerdata.docptr), document, old_rtype_doc,
                workerdata.oldDocId, workerdata.newDocId, source,
                new_timestamp, workerdata.updateType, workerdata.scdType))
            continue;

        if (workerdata.scdType == INSERT_SCD || workerdata.oldDocId == 0)
        {
            insertDoc_(workerid, document, workerdata.timestamp);
        }
        else
        {
            if (!updateDoc_(workerid, workerdata.oldDocId, document, old_rtype_doc, workerdata.timestamp, workerdata.updateType))
                continue;
            ++numUpdatedDocs_;
        }
    }

    } catch (const boost::thread_interrupted& e)
    {
        index_thread_status_[workerid] = false;
        asynchronousTasks_[workerid]->clear();
        flushUpdateBuffer_(workerid);
        LOG(INFO) << "index thread worker exited : " << workerid;
    }
}

bool IndexWorker::insertOrUpdateSCD_(
        ScdParser& parser,
        SCD_TYPE scdType,
        uint32_t numdoc,
        time_t timestamp)
{
    CREATE_SCOPED_PROFILER (insertOrUpdateSCD, "IndexWorker", "IndexWorker::insertOrUpdateSCD_");

    uint32_t n = 0;
    long lastOffset = 0;
    Document document;
    Document old_rtype_doc;
    //IndexerDocument indexDocument, oldIndexDocument;

    for (ScdParser::iterator doc_iter = parser.begin();
            doc_iter != parser.end(); ++doc_iter, ++n)
    {
        if (*doc_iter == NULL)
        {
            LOG(WARNING) << "SCD File not valid.";
            return false;
        }

        indexProgress_.currentFilePos_ += doc_iter.getOffset() - lastOffset;
        indexProgress_.totalFilePos_ += doc_iter.getOffset() - lastOffset;
        lastOffset = doc_iter.getOffset();
        if (0 < numdoc && numdoc <= n)
            break;

        if (n % 1000 == 0)
        {
            indexProgress_.getIndexingStatus(indexStatus_);
            indexStatus_.progress_ = indexProgress_.getTotalPercent();
            indexStatus_.elapsedTime_ = boost::posix_time::seconds((int)indexProgress_.getElapsed());
            indexStatus_.leftTime_ =  boost::posix_time::seconds((int)indexProgress_.getLeft());
        }

        SCDDocPtr docptr = *doc_iter;
        if (docptr->empty()) continue;

        int workerid = -1;
        docid_t docId;
        docid_t oldId = 0;
        UpdateType updateType = INSERT;

        std::string source = "";
        SCDDoc::const_iterator p = docptr->begin();
        for (; p != docptr->end(); p++)
        {
            if (boost::iequals(p->first, DOCID))
            {
                uint128_t scdDocId = Utilities::md5ToUint128(p->second);

                if (!prepareDocIdAndUpdateType_(scdDocId, *docptr, scdType,
                        oldId, docId, updateType))
                    break;

                workerid = scdDocId % asynchronousTasks_.size();
                continue;
            }
            if (!bundleConfig_->productSourceField_.empty()
                && boost::iequals(p->first, bundleConfig_->productSourceField_))
            {
                source = propstr_to_str(p->second);
            }
        }

        if (workerid == -1)
            continue;

        if (is_real_time_)
        {
            LOG(INFO) << "indexing in single thread while in real time mode";
            // real time can not using multi thread because of the inverted index in 
            // the real time can not handle the out-of-order docid list.
            std::string source = "";
            time_t new_timestamp = timestamp;
            document.clear();
            old_rtype_doc.clear();

            if (!prepareDocument_(*(docptr), document, old_rtype_doc,
                    oldId, docId, source, new_timestamp, updateType, scdType))
                continue;

            if (scdType == INSERT_SCD || oldId == 0)
            {
                insertDoc_(0, document, timestamp, true);
            }
            else
            {
                if (!updateDoc_(0, oldId, document, old_rtype_doc, timestamp, updateType, true))
                    continue;
                ++numUpdatedDocs_;
            }
        }
        else
        {
            asynchronousTasks_[workerid]->push(IndexDocInfo(docptr, oldId, docId,
                    scdType, updateType, timestamp));
        }
        if (!source.empty())
        {
            ++productSourceCount_[source];
        }

        // interrupt when closing the process
        boost::this_thread::interruption_point();
    } // end of for loop for all documents

    if (is_real_time_)
    {
        //flushUpdateBuffer_(0);
    }
    else
    {
        for (size_t i = 0; i < asynchronousTasks_.size(); ++i)
        {
            asynchronousTasks_[i]->push(IndexDocInfo());
        }
        // wait flush finish.
        for (size_t i = 0; i < asynchronousTasks_.size(); ++i)
        {
            while(!updateBuffer_[i].empty() || !asynchronousTasks_[i]->empty() ||
                index_thread_status_[i])
            {
                sleep(2);
                //LOG(INFO) << "index thread still buffering ... ";
            }
        }
        sleep(1);
        LOG(INFO) << "scd finished index for all thread.";
    }
    return true;
}

bool IndexWorker::createInsertDocId_(
        const uint128_t& scdDocId,
        docid_t& newId)
{
    CREATE_SCOPED_PROFILER (proCreateInsertDocId, "IndexWorker", "IndexWorker::createInsertDocId_");
    docid_t docId = 0;

    // already converted
    if (idManager_->getDocIdByDocName(scdDocId, docId))
    {
        if (documentManager_->isDeleted(docId))
        {
            idManager_->updateDocIdByDocName(scdDocId, docId);
        }
        else
        {
            //LOG(WARNING) << "duplicated doc id " << docId << ", it has already been inserted before";
            return false;
        }
    }

    if (docId <= documentManager_->getMaxDocId())
    {
        LOG(WARNING) << "skip insert doc id " << docId << ", it is less than max DocId" <<
            documentManager_->getMaxDocId();
        return false;
    }

    newId = docId;
    return true;
}

bool IndexWorker::deleteSCD_(ScdParser& parser, time_t timestamp)
{
    std::vector<ScdPropertyValueType> rawDocIDList;
    if (!parser.getDocIdList(rawDocIDList))
    {
        LOG(WARNING) << "SCD File not valid.";
        return false;
    }

    //get the docIds for deleting
    std::vector<docid_t> docIdList;
    docIdList.reserve(rawDocIDList.size());
    indexProgress_.currentFileSize_ =rawDocIDList.size();
    indexProgress_.currentFilePos_ = 0;
    for (std::vector<ScdPropertyValueType>::iterator iter = rawDocIDList.begin();
            iter != rawDocIDList.end(); ++iter)
    {
        docid_t docId;
        std::string docid_str = propstr_to_str(*iter);
        if (idManager_->getDocIdByDocName(Utilities::md5ToUint128(docid_str), docId, false))
        {
            docIdList.push_back(docId);
            documentManager_->last_delete_docid_.push_back(docId);
        }
    }
    std::sort(docIdList.begin(), docIdList.end());

    miningTaskService_->EnsureHasDeletedDocDuringMining();

    //process delete document in index manager
    for (std::vector<docid_t>::iterator iter = docIdList.begin(); iter
            != docIdList.end(); ++iter)
    {
        if (numDeletedDocs_ % 1000 == 0)
        {
            indexProgress_.getIndexingStatus(indexStatus_);
            indexStatus_.progress_ = indexProgress_.getTotalPercent();
            indexStatus_.elapsedTime_ = boost::posix_time::seconds((int)indexProgress_.getElapsed());
            indexStatus_.leftTime_ =  boost::posix_time::seconds((int)indexProgress_.getLeft());
        }

        if (!bundleConfig_->productSourceField_.empty())
        {
            PropertyValue value;
            if (documentManager_->getPropertyValue(*iter, bundleConfig_->productSourceField_, value))
            {
                std::string source("");
                if (getPropertyValue_(value, source))
                {
                    ++productSourceCount_[source];
                }
                else
                {
                    return false;
                }
            }
        }

        //marks delete key to true in DB

        if (!deleteDoc_(*iter, timestamp))
        {
            LOG(WARNING) << "Cannot delete removed Document. docid. " << *iter;
            continue;
        }
        ++indexProgress_.currentFilePos_;

    }

    // interrupt when closing the process
    boost::this_thread::interruption_point();

    return true;
}

bool IndexWorker::insertDoc_(
        size_t wid,
        Document& document,
        time_t timestamp,
        bool immediately)
{
    if(hooker_)
    {
        if (!hooker_->HookInsert(document, timestamp))
            return false;
    }

    if (immediately)
    {
        return doInsertDoc_(document, timestamp);
    }

    ///updateBuffer_ is used to change random IO in DocumentManager to sequential IO
    UpdateBufferDataType& updateData = updateBuffer_[wid][document.getId()];
    updateData.get<0>() = INSERT;
    updateData.get<1>().swap(document);
    updateData.get<2>() = 0;
    updateData.get<3>() = timestamp;
    if (updateBuffer_[wid].size() >= UPDATE_BUFFER_CAPACITY)
    {
        flushUpdateBuffer_(wid);
    }
    return true;
}

bool IndexWorker::doInsertDoc_(Document& document, time_t timestamp)
{
    CREATE_PROFILER(proDocumentIndexing, "IndexWorker", "IndexWorker : InsertDocument")
    CREATE_PROFILER(proIndexing, "IndexWorker", "IndexWorker : indexing")

    START_PROFILER(proDocumentIndexing);
    if (documentManager_->insertDocument(document))
    {
        STOP_PROFILER(proDocumentIndexing);

        START_PROFILER(proIndexing);
        inc_supported_index_manager_.insertDocument(document, timestamp);
        STOP_PROFILER(proIndexing);
        //indexStatus_.numDocs_ = inc_supported_index_manager_.numDocs();
        return true;
    }
    return false;
}

bool IndexWorker::updateDoc_(
        size_t wid,
        docid_t oldId,
        Document& document,
        const Document& old_rtype_doc,
        time_t timestamp,
        IndexWorker::UpdateType updateType,
        bool immediately)
{
    CREATE_SCOPED_PROFILER (proDocumentUpdating, "IndexWorker", "IndexWorker::UpdateDocument");
    if (INSERT == updateType)
        return insertDoc_(wid, document, timestamp, immediately);

    if (hooker_)
    {
        ///Notice: the success of HookUpdate will not affect following update
        hooker_->HookUpdate(document, oldId, timestamp);
    }

    if (immediately)
        return doUpdateDoc_(oldId, document, old_rtype_doc, updateType, timestamp);

    ///updateBuffer_ is used to change random IO in DocumentManager to sequential IO
    std::pair<UpdateBufferType::iterator, bool> insertResult =
        updateBuffer_[wid].insert(document.getId(), UpdateBufferDataType());

    UpdateBufferDataType& updateData = insertResult.first.data();

    if (!insertResult.second)
    {
        LOG(INFO) << "duplicated update for doc: " << oldId;
        doUpdateDoc_(updateData.get<2>(), updateData.get<1>(),
            updateData.get<4>(), updateData.get<0>(), updateData.get<3>());
    }

    updateData.get<0>() = updateType;
    updateData.get<1>().swap(document);
    updateData.get<2>() = oldId;
    updateData.get<3>() = timestamp;
    updateData.get<4>() = old_rtype_doc;

    if (updateBuffer_[wid].size() >= UPDATE_BUFFER_CAPACITY)
    {
        flushUpdateBuffer_(wid);
    }

    return true;
}

bool IndexWorker::doUpdateDoc_(
        docid_t oldId,
        Document& document,
        const Document& old_rtype_doc,
        IndexWorker::UpdateType updateType,
        time_t timestamp)
{
    Document oldDoc;
    documentManager_->getDocument(oldId, oldDoc, true);

    switch (updateType)
    {
    case GENERAL:
    {
        mergeDocument_(oldId, document);
        if (!documentManager_->removeDocument(oldId))
        {
            LOG(WARNING) << "document " << oldId << " is already deleted";
        }
        else
        {
            miningTaskService_->EnsureHasDeletedDocDuringMining();
        }
        if (!documentManager_->insertDocument(document))
        {
            LOG(ERROR) << "Document Insert Failed in SDB. " << document.property("DOCID");
            return false;
        }
        break;
    }
    case REPLACE:
    {
        mergeDocument_(oldId, document);
        if(!documentManager_->updateDocument(document))
            return false;
        break;
    }

    default:
    {
        break;
    }
    }

    inc_supported_index_manager_.updateDocument(oldDoc, old_rtype_doc, document, updateType, timestamp);
    return true;
}

void IndexWorker::flushUpdateBuffer_(size_t wid)
{
    for (UpdateBufferType::iterator it = updateBuffer_[wid].begin();
            it != updateBuffer_[wid].end(); ++it)
    {
        UpdateBufferDataType& updateData = it->second;
        uint32_t oldId = updateData.get<2>();
        Document oldDoc;
        documentManager_->getDocument(oldId, oldDoc, true);
        bool need_update_index = true;

        switch (updateData.get<0>())
        {
        case INSERT:
        {
            if (documentManager_->insertDocument(updateData.get<1>()))
            {
                //indexStatus_.numDocs_ = inc_supported_index_manager_.numDocs();
            }
            else
            {
                need_update_index = false;
                LOG(ERROR) << "Document Insert Failed in SDB. " << updateData.get<1>().property("DOCID");
            }
            break;
        }

        case GENERAL:
        {
            if (!mergeDocument_(oldId, updateData.get<1>()))
            {
                //updateData.get<0>() = UNKNOWN;
                LOG(INFO) << "doc id: " << it->first << " merger failed in flush general.";
                break;
            }

            if (documentManager_->removeDocument(oldId))
            {
                miningTaskService_->EnsureHasDeletedDocDuringMining();
            }

            if (!documentManager_->insertDocument(updateData.get<1>()))
            {
                LOG(ERROR) << "Document Insert Failed in SDB. " << updateData.get<1>().property("DOCID");
            }
            break;
        }
        case REPLACE:
        {
            uint32_t oldId = updateData.get<2>();
            if (mergeDocument_(oldId, updateData.get<1>()))
                documentManager_->updateDocument(updateData.get<1>());
            break;
        }

        default:
            break;
        }

        if (need_update_index)
        {
            if (updateData.get<0>() == INSERT)
            {
                inc_supported_index_manager_.insertDocument(updateData.get<1>(),
                    updateData.get<3>());
            }
            else
            {
                inc_supported_index_manager_.updateDocument(oldDoc, updateData.get<4>(),
                    updateData.get<1>(), updateData.get<0>(), updateData.get<3>());
            }
        }
    }

    UpdateBufferType().swap(updateBuffer_[wid]);
}

bool IndexWorker::deleteDoc_(docid_t docid, time_t timestamp)
{
    if (hooker_)
    {
        if (!hooker_->HookDelete(docid, timestamp))
            return false;
    }

    CREATE_SCOPED_PROFILER (proDocumentDeleting, "IndexWorker", "IndexWorker::DeleteDocument");
    if (documentManager_->removeDocument(docid))
    {
        inc_supported_index_manager_.removeDocument(docid, timestamp);
        ++numDeletedDocs_;
        //indexStatus_.numDocs_ = inc_supported_index_manager_.numDocs();
        return true;
    }
    return false;
}

void IndexWorker::savePriceHistory_(int op)
{
}

void IndexWorker::saveSourceCount_(SCD_TYPE scdType)
{
    if (bundleConfig_->productSourceField_.empty())
        return;

    boost::posix_time::ptime now = boost::posix_time::microsec_clock::local_time();
    for (map<std::string, uint32_t>::const_iterator iter = productSourceCount_.begin();
            iter != productSourceCount_.end(); ++iter)
    {
        ProductCount productCount;
        productCount.setSource(iter->first);
        productCount.setCollection(bundleConfig_->collectionName_);
        productCount.setNum(iter->second);
        productCount.setFlag(ScdParser::SCD_TYPE_NAMES[scdType]);
        productCount.setTimeStamp(now);
        productCount.save();
    }
}

bool IndexWorker::prepareDocIdAndUpdateType_(const izenelib::util::UString& scdDocIdUStr,
    const SCDDoc& scddoc, SCD_TYPE scdType,
    docid_t& oldDocId, docid_t& newDocId, IndexWorker::UpdateType& updateType)
{
    std::string scdDocIdStr;
    scdDocIdUStr.convertString(scdDocIdStr, bundleConfig_->encoding_);
    return prepareDocIdAndUpdateType_(scdDocIdStr, scddoc, scdType, oldDocId, newDocId, updateType);
}

bool IndexWorker::prepareDocIdAndUpdateType_(const std::string& scdDocIdStr,
    const SCDDoc& scddoc, SCD_TYPE scdType,
    docid_t& oldDocId, docid_t& newDocId, IndexWorker::UpdateType& updateType)
{
    uint128_t scdDocId = Utilities::md5ToUint128(scdDocIdStr);
    return prepareDocIdAndUpdateType_(scdDocId, scddoc, scdType, oldDocId, newDocId, updateType);
}

bool IndexWorker::prepareDocIdAndUpdateType_(const uint128_t& scdDocId,
    const SCDDoc& scddoc, SCD_TYPE scdType,
    docid_t& oldDocId, docid_t& newDocId, IndexWorker::UpdateType& updateType)
{
    if (scdType != INSERT_SCD)
    {
        updateType = checkUpdateType_(scdDocId, scddoc, oldDocId, newDocId, scdType);

        if (updateType == INSERT && scdType == RTYPE_SCD)
            return false;

        if (updateType == RTYPE)
        {
            if (documentManager_->isDeleted(oldDocId))
                return false;
        }
    }
    else if (!createInsertDocId_(scdDocId, newDocId))
    {
        return false;
    }
    return true;
}

bool IndexWorker::prepareDocument_(
        const SCDDoc& doc,
        Document& document,
        Document& old_rtype_doc,
        const docid_t& oldId,
        const docid_t& docId,
        std::string& source,
        time_t& timestamp,
        const IndexWorker::UpdateType& updateType,
        SCD_TYPE scdType)
{
    CREATE_SCOPED_PROFILER (preparedocument, "IndexWorker", "IndexWorker::prepareDocument_");
    CREATE_PROFILER (pid_date, "IndexWorker", "IndexWorker::prepareDocument__::DATE");

    //docid_t docId = 0;
    string fieldStr;
    vector<CharacterOffset> sentenceOffsetList;
    AnalysisInfo analysisInfo;
    if (doc.empty()) return false;
    // the iterator is not const because the p-second value may change
    // due to the maxlen setting

    SCDDoc::const_iterator p = doc.begin();
    bool dateExistInSCD = false;

    PropertyConfig tempPropertyConfig;

    for (; p != doc.end(); p++)
    {
        const std::string& fieldStr = p->first;
        tempPropertyConfig.propertyName_ = fieldStr;
        IndexBundleSchema::const_iterator iter = bundleConfig_->indexSchema_.find(tempPropertyConfig);
        bool isIndexSchema = (iter != bundleConfig_->indexSchema_.end());
		
        const ScdPropertyValueType & propertyValueU = p->second; // preventing copy

        if (boost::iequals(fieldStr, DOCID) && isIndexSchema)
        {
            document.setId(docId);
            PropertyValue propData(propertyValueU);
            document.property(fieldStr).swap(propData);

            if (oldId && oldId != docId)
                documentManager_->moveRTypeValues(oldId, docId);
        }
        else if (boost::iequals(fieldStr, dateProperty_.getName()))
        {
            /// format <DATE>20091009163011
            START_PROFILER(pid_date);
            dateExistInSCD = true;
            time_t ts = Utilities::createTimeStampInSeconds(propertyValueU);
            boost::shared_ptr<NumericPropertyTableBase>& datePropertyTable = documentManager_->getNumericPropertyTable(dateProperty_.getName());
            if (datePropertyTable)
            {
                std::string rtypevalue;
                datePropertyTable->getStringValue(docId, rtypevalue);
                old_rtype_doc.property(fieldStr) = rtypevalue;
                datePropertyTable->setInt64Value(docId, ts);
            }
            else
            {
                PropertyValue propData(propertyValueU);
                document.property(dateProperty_.getName()).swap(propData);
            }
            STOP_PROFILER(pid_date);
        }
        else if (isIndexSchema)
        {
            switch (iter->getType())
            {
            case STRING_PROPERTY_TYPE:
                {
                    if (iter->isRTypeString())
                    {
                        boost::shared_ptr<RTypeStringPropTable>& rtypeprop = documentManager_->getRTypeStringPropTable(iter->getName());
                        const std::string fieldValue = propstr_to_str(propertyValueU);

                        std::string rtypevalue;
                        rtypeprop->getRTypeString(docId, rtypevalue);
                        old_rtype_doc.property(fieldStr) = rtypevalue;

                        rtypeprop->updateRTypeString(docId, fieldValue);
                    }
                    else
                    {
                        PropertyValue propData(propertyValueU);
                        document.property(fieldStr).swap(propData);
                        analysisInfo.clear();
                        analysisInfo = iter->getAnalysisInfo();
                        if (!analysisInfo.analyzerId_.empty())
                        {
                            CREATE_SCOPED_PROFILER (prepare_summary, "IndexWorker", "IndexWorker::prepareDocument_::Summary");
                            unsigned int numOfSummary = 0;
                            if ((iter->getIsSnippet() || iter->getIsSummary()))
                            {
                                if (iter->getIsSummary())
                                {
                                    numOfSummary = iter->getSummaryNum();
                                    if (numOfSummary <= 0)
                                        numOfSummary = 1; //atleast one sentence required for summary
                                }

                                if (!makeSentenceBlocks_(propstr_to_ustr(propertyValueU), iter->getDisplayLength(),
                                        numOfSummary, sentenceOffsetList))
                                {
                                    LOG(ERROR) << "Make Sentence Blocks Failes ";
                                }
                                PropertyValue propData(sentenceOffsetList);
                                document.property(fieldStr + ".blocks").swap(propData);
                            }
                        }
                    }
                }
                break;
            case SUBDOC_PROPERTY_TYPE:
                {
                    PropertyValue propData(propertyValueU);
                    document.property(fieldStr).swap(propData);
                }
                break;
            case INT32_PROPERTY_TYPE:
            case FLOAT_PROPERTY_TYPE:
            case INT8_PROPERTY_TYPE:
            case INT16_PROPERTY_TYPE:
            case INT64_PROPERTY_TYPE:
            case DOUBLE_PROPERTY_TYPE:
            case NOMINAL_PROPERTY_TYPE:
                if (iter->getIsFilter() && !iter->getIsMultiValue())
                {
                    boost::shared_ptr<NumericPropertyTableBase>& numericPropertyTable = documentManager_->getNumericPropertyTable(iter->getName());
                    const std::string fieldValue = propstr_to_str(propertyValueU);

                    std::string rtypevalue;
                    numericPropertyTable->getStringValue(docId, rtypevalue);
                    old_rtype_doc.property(fieldStr) = rtypevalue;
                    
                    numericPropertyTable->setStringValue(docId, fieldValue);
                }
                else
                {
                    PropertyValue propData(propertyValueU);
                    document.property(fieldStr).swap(propData);
                }
                if (updateType == RTYPE)
                    documentManager_->RtypeDocidPros_.insert(fieldStr);
                break;

            case DATETIME_PROPERTY_TYPE:
                if (iter->getIsFilter() && !iter->getIsMultiValue())
                {
                    time_t ts = Utilities::createTimeStampInSeconds(propertyValueU);
                    boost::shared_ptr<NumericPropertyTableBase>& datePropertyTable = documentManager_->getNumericPropertyTable(iter->getName());

                    std::string rtypevalue;
                    datePropertyTable->getStringValue(docId, rtypevalue);
                    old_rtype_doc.property(fieldStr) = rtypevalue;

                    datePropertyTable->setInt64Value(docId, ts);
                }
                else
                {
                    PropertyValue propData(propertyValueU);
                    document.property(fieldStr).swap(propData);
                }
                if (updateType == RTYPE)
                    documentManager_->RtypeDocidPros_.insert(fieldStr);
                break;

            default:
                break;
            }

        }
    }

    if (dateExistInSCD)
    {
        timestamp = -1;
    }
    else
    {
        time_t tm = timestamp / 1000000;
        boost::shared_ptr<NumericPropertyTableBase>& numericPropertyTable
            = documentManager_->getNumericPropertyTable(dateProperty_.getName());

        std::string rtypevalue;
        numericPropertyTable->getStringValue(docId, rtypevalue);
        old_rtype_doc.property(dateProperty_.getName()) = rtypevalue;

        numericPropertyTable->setInt64Value(docId, tm);
    }

    return true;
}

bool IndexWorker::mergeDocument_(
        docid_t oldId,
        Document& doc)
{
    ///We need to retrieve old document, and then merge them with new document data
    docid_t newId = doc.getId();
    Document oldDoc;
    if (!documentManager_->getDocument(oldId, oldDoc, true))
    {
        return false;
    }
    oldDoc.copyPropertiesFromDocument(doc);

    doc.swap(oldDoc);
    doc.setId(newId);
    return true;
}

IndexWorker::UpdateType IndexWorker::checkUpdateType_(
        const uint128_t& scdDocId,
        const SCDDoc& doc,
        docid_t& oldId,
        docid_t& docId,
        SCD_TYPE scdType)
{
    if (!idManager_->getDocIdByDocName(scdDocId, oldId, false))
    {
        idManager_->updateDocIdByDocName(scdDocId, docId);
        return INSERT;
    }

    if (scdType == RTYPE_SCD)
    {
        docId = oldId;
        return RTYPE;
    }

    PropertyConfig tempPropertyConfig;
    SCDDoc::const_iterator p = doc.begin();
    for (; p != doc.end(); ++p)
    {
        const string& fieldName = p->first;
        const ScdPropertyValueType& propertyValueU = p->second;
        if (boost::iequals(fieldName, DOCID))
            continue;
        if (propertyValueU.empty())
            continue;

        tempPropertyConfig.propertyName_ = fieldName;
        IndexBundleSchema::iterator iter = bundleConfig_->indexSchema_.find(tempPropertyConfig);

        if (iter == bundleConfig_->indexSchema_.end())
            continue;

        if (iter->isRTypeString())
            continue;

        if (iter->isIndex())
        {
            if (iter->isAnalyzed())
            {
                idManager_->updateDocIdByDocName(scdDocId, docId);
                return GENERAL;
            }
            else if (iter->getIsFilter() && iter->getType() != STRING_PROPERTY_TYPE)
            {
                continue;
            }
        }
        idManager_->updateDocIdByDocName(scdDocId, docId);
        return GENERAL;
    }

    docId = oldId;
    return RTYPE;
}

bool IndexWorker::makeSentenceBlocks_(
        const izenelib::util::UString& text,
        const unsigned int maxDisplayLength,
        const unsigned int numOfSummary,
        vector<CharacterOffset>& sentenceOffsetList)
{
    sentenceOffsetList.clear();
    if (!summarizer_.getOffsetPairs(text, maxDisplayLength, numOfSummary, sentenceOffsetList))
    {
        return false;
    }
    return true;
}

size_t IndexWorker::getTotalScdSize_(const std::vector<std::string>& scdlist)
{
    ScdParser parser(bundleConfig_->encoding_);
    size_t sizeInBytes = 0;
    for (size_t i = 0; i < scdlist.size(); ++i)
    {
        if (bfs::is_regular_file(scdlist[i]))
        {
            parser.load(scdlist[i]);
            sizeInBytes += parser.getFileSize();
        }
    }
    return sizeInBytes;
}

void IndexWorker::value2SCDDoc(const Value& value, SCDDoc& scddoc)
{
    const Value::ObjectType& objectValue = value.getObject();
    scddoc.resize(objectValue.size());

    std::size_t propertyId = 1;
    for (Value::ObjectType::const_iterator it = objectValue.begin();
            it != objectValue.end(); ++it, ++propertyId)
    {
        std::size_t insertto = propertyId;
        // the first position for SCDDoc must be preserved for the DOCID,
        // since the other property must be written after the docid in the LAInput.
        if (boost::iequals(asString(it->first), DOCID))
        {
            insertto = 0;
            --propertyId;
        }
        scddoc[insertto].first.assign(asString(it->first));
        scddoc[insertto].second.assign(asString(it->second));
    }
}

void IndexWorker::document2SCDDoc(const Document& document, SCDDoc& scddoc)
{
    scddoc.resize(document.getPropertySize());

    std::size_t propertyId = 1;
    Document::property_const_iterator it = document.propertyBegin();
    for (; it != document.propertyEnd(); ++it,++propertyId)
    {
        std::size_t insertto = propertyId;
        // the first position for SCDDoc must be preserved for the DOCID,
        const std::string& propertyName = it->first;

        try
        {
            const Document::doc_prop_value_strtype& propValue = document.property(it->first).getPropertyStrValue();
            if (boost::iequals(propertyName, DOCID))
            {
                insertto = 0;
                --propertyId;
            }
            scddoc[insertto].first.assign(it->first);
            scddoc[insertto].second.assign(propValue);
        }
        catch(const boost::bad_get& e)
        {
        }
    }
}

void IndexWorker::clearMasterCache_()
{
    if (bundleConfig_->isWorkerNode())
    {
        NotifyMSG msg;
        msg.collection = bundleConfig_->collectionName_;
        msg.method = "CLEAR_SEARCH_CACHE";
        MasterNotifier::get()->notify(msg);
    }
}

}
