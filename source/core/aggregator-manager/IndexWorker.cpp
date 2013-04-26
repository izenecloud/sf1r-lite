#include "IndexWorker.h"
#include "SearchWorker.h"
#include "WorkerHelper.h"

#include <index-manager/IndexManager.h>
#include <index-manager/IndexHooker.h>
#include <index-manager/IndexModeSelector.h>
#include <search-manager/SearchManager.h>
#include <mining-manager/MiningManager.h>
#include <common/NumericPropertyTable.h>
#include <common/NumericRangePropertyTable.h>
#include <common/RTypeStringPropTable.h>
#include <document-manager/DocumentManager.h>
#include <la-manager/LAManager.h>
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
const char* SUMMARY_CONTROL_FLAG = "b5m_control";
const char* SCD_BACKUP_DIR = "backup";
const std::string DOCID("DOCID");
const std::string DATE("DATE");
const size_t UPDATE_BUFFER_CAPACITY = 8192;
PropertyConfig tempPropertyConfig;
}

IndexWorker::IndexWorker(
        IndexBundleConfiguration* bundleConfig,
        DirectoryRotator& directoryRotator,
        boost::shared_ptr<IndexManager> indexManager)
    : bundleConfig_(bundleConfig)
    , miningTaskService_(NULL)
    , recommendTaskService_(NULL)
    , indexManager_(indexManager)
    , directoryRotator_(directoryRotator)
    , scd_writer_(new ScdWriterController(bundleConfig_->logSCDPath()))
    , collectionId_(1)
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

    indexStatus_.numDocs_ = indexManager_->numDocs();

    config_tool::buildPropertyAliasMap(bundleConfig_->indexSchema_, propertyAliasMap_);

    ///propertyId starts from 1
    laInputs_.resize(bundleConfig_->indexSchema_.size() + 1);

    for (IndexBundleSchema::const_iterator iter = indexSchema.begin(), iterEnd = indexSchema.end();
            iter != iterEnd; ++iter)
    {
        laInputs_[iter->getPropertyId()].reset(new LAInput);
    }

    createPropertyList_();
    scd_writer_->SetFlushLimit(500);
    scheduleOptimizeTask();
}

IndexWorker::~IndexWorker()
{
    delete scd_writer_;
    if (!optimizeJobDesc_.empty())
        izenelib::util::Scheduler::removeJob(optimizeJobDesc_);
}

void IndexWorker::HookDistributeRequestForIndex(int hooktype, const std::string& reqdata, bool& result)
{
    LOG(INFO) << "new api request from shard, packeddata len: " << reqdata.size();
    MasterManagerBase::get()->pushWriteReq(reqdata, "api_from_shard");

    //distribute_req_hooker_->setHook(hooktype, reqdata);
    //distribute_req_hooker_->hookCurrentReq(reqdata);
    //distribute_req_hooker_->processLocalBegin();
    LOG(INFO) << "got hook request on the worker in indexworker." << reqdata;
    result = true;
}

void IndexWorker::flush(bool mergeBarrel)
{
    LOG(INFO) << "Begin flushing in IndexWorker ....";
    documentManager_->flush();
    idManager_->flush();
    LOG(INFO) << "Begin flushing indexManager_ in IndexWorker ....";
    indexManager_->flush();
    if (mergeBarrel)
    {
        LOG(INFO) << "Begin merge index ....";
        indexManager_->optimizeIndex();
        indexManager_->waitForMergeFinish();
        LOG(INFO) << "End merge index ....";
    }
    if (miningTaskService_)
        miningTaskService_->flush();
    LOG(INFO) << "Flushing finished in IndexWorker";
}

bool IndexWorker::reload()
{
    if(!documentManager_->reload())
        return false;

    idManager_->close();

    indexManager_->setDirty();
    indexManager_->getIndexReader();
    return true;
}

void IndexWorker::index(unsigned int numdoc, bool& result)
{
    result = true;
    if (distribute_req_hooker_->isRunningPrimary())
    {
        // in distributed, all write api need be called in sync.
        if (!distribute_req_hooker_->isHooked())
        {
            task_type task = boost::bind(&IndexWorker::buildCollection, this, numdoc);
            JobScheduler::get()->addTask(task, bundleConfig_->collectionName_);
        }
        else
        {
            result = buildCollection(numdoc);
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

bool IndexWorker::buildCollection(unsigned int numdoc)
{
    DISTRIBUTE_WRITE_BEGIN;
    DISTRIBUTE_WRITE_CHECK_VALID_RETURN;

    ///If current directory is the one rotated from the backup directory,
    ///there should exist some missed SCDs since the last backup time,
    ///so we move those SCDs from backup directory, so that these data
    ///could be recovered through indexing
    //recoverSCD_();

    string scdPath = bundleConfig_->indexSCDPath();
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

    // push scd files to replicas
    for (size_t file_index = 0; file_index < scdList.size(); ++file_index)
    {
        if(DistributeFileSyncMgr::get()->pushFileToAllReplicas(scdList[file_index],
                scdList[file_index]))
        {
            LOG(INFO) << "Transfer index scd to the replicas finished for: " << scdList[file_index];
        }
        else
        {
            LOG(WARNING) << "push index scd file to the replicas failed for:" << scdList[file_index];
        }
    }

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

    //recoverSCD_();
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
    size_t currTotalSCDSize = indexProgress_.totalFileSize_/(1024*1024);
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

    if (documentManager_->getMaxDocId() < 1)
    {
        if (miningTaskService_->getMiningBundleConfig()->mining_schema_.summarization_enable
            && miningTaskService_->getMiningBundleConfig()->mining_schema_.summarization_schema.isSyncSCDOnly)
        {
            std::string control_scd_path_ = bundleConfig_->indexSCDPath() + "/full";
            fstream outControlFile;
            outControlFile.open(control_scd_path_.c_str(), ios::out);
            outControlFile<<"full"<<endl;
            outControlFile.close();

            SynchroData syncControlFile;
            syncControlFile.setValue(SynchroData::KEY_COLLECTION, bundleConfig_->collectionName_);
            syncControlFile.setValue(SynchroData::KEY_DATA_TYPE, SynchroData::COMMENT_TYPE_FLAG);
            syncControlFile.setValue(SynchroData::KEY_DATA_PATH, control_scd_path_.c_str());
            SynchroProducerPtr syncProducer = SynchroFactory::getProducer(SUMMARY_CONTROL_FLAG);
            if (syncProducer->produce(syncControlFile, boost::bind(boost::filesystem::remove_all, control_scd_path_.c_str())))
            {
                syncProducer->wait(10);
            }
            else
            {
                LOG(WARNING) << "produce syncData error";
            }
        }
    }

    indexProgress_.currentFileIdx = 1;


    //here, try to set the index mode(default[batch] or realtime)
    //The threshold is set to the scd_file_size/exist_doc_num, if smaller or equal than this threshold then realtime mode will turn on.
    //when the scd file size(M) larger than max_realtime_msize, the default mode will turn on while ignore the threshold above.
    long max_realtime_msize = 50L; //set to 50M
    double threshold = 50.0/500000.0;
    IndexModeSelector index_mode_selector(indexManager_, threshold, max_realtime_msize);
    index_mode_selector.TrySetIndexMode(indexProgress_.totalFileSize_);

    if (!indexManager_->isRealTime())
    {
        indexManager_->setIndexMode("realtime");
        indexManager_->flush();
        indexManager_->deletebinlog();
        indexManager_->setIndexMode("default");
    }

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
            index_mode_selector.TryCommit();
            //indexManager_->optimizeIndex();
        }
        catch (std::exception& e)
        {
            LOG(WARNING) << "exception in indexing : " << e.what();
            indexProgress_.getIndexingStatus(indexStatus_);
            indexProgress_.reset();
            return false;
        }

        const boost::shared_ptr<Directory>& currentDir = directoryRotator_.currentDirectory();

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
                std::cout<<"[IndexWorker] Hooker Finish failed."<<std::endl;
                return false;
            }
            std::cout<<"[IndexWorker] Hooker Finished."<<std::endl;
        }

        if (miningTaskService_)
        {
            indexManager_->pauseMerge();
            miningTaskService_->DoMiningCollection(timestamp);
            indexManager_->resumeMerge();
        }
    }
    catch (std::exception& e)
    {
        LOG(WARNING) << "exception in mining: " << e.what();
        indexProgress_.getIndexingStatus(indexStatus_);
        indexProgress_.reset();
        return false;
    }

    indexManager_->getIndexReader();

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

    if (requireBackup_(currTotalSCDSize))
    {
        ///When index can support binlog, this step is not necessary
        ///It means when work under realtime mode, the benefits of reduced merging
        ///caused by frequently updating can not be achieved if Backup is required
        index_mode_selector.ForceCommit();
        if (!backup_())
            return false;
        totalSCDSizeSinceLastBackup_ = 0;
    }

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

    docid_t oldId = 0;
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

        // update docid
        std::string docidName("DOCID");
        izenelib::util::UString docidValueU;
        if (!document.getProperty(docidName, docidValueU))
        {
            //LOG(WARNING) << "skip doc which has no DOCID property: " << curDocId;
            continue;
        }

        docid_t newDocId;
        std::string docid_str;
        docidValueU.convertString(docid_str, izenelib::util::UString::UTF_8);
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

        IndexerDocument indexDocument;
        time_t new_timestamp = -1;
        prepareIndexDocument_(oldId, new_timestamp, document, indexDocument);

        if (!insertDoc_(document, indexDocument, timestamp))
            continue;

        insertedCount++;
        if (insertedCount % 10000 == 0)
        {
            LOG(INFO) << "inserted doc number: " << insertedCount;
        }

        // interrupt when closing the process
        boost::this_thread::interruption_point();
    }
    flushUpdateBuffer_();

    LOG(INFO) << "inserted doc number: " << insertedCount << ", total: " << maxDocId;
    LOG(INFO) << "Indexing Finished";

    documentManager_->flush();
    idManager_->flush();
    indexManager_->flush();

    if (miningTaskService_)
    {
        indexManager_->pauseMerge();
        miningTaskService_->DoMiningCollection(timestamp);
        indexManager_->resumeMerge();
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

    if (!backup_())
    {
        return false;
    }

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

    indexManager_->optimizeIndex();

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
        indexManager_->optimizeIndex();

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
            if (docLimit != 0 && (indexManager_->numDocs()) % docLimit == 0)
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
    IndexerDocument indexDocument, oldIndexDocument;
    docid_t oldId = 0;
    std::string source = "";
    IndexWorker::UpdateType updateType;
    time_t timestamp = reqlog.timestamp;
    if (!prepareDocument_(scddoc, document, indexDocument, oldIndexDocument, oldId, source, timestamp, updateType, INSERT_SCD))
        return false;

    if (!indexManager_->isRealTime())
    	indexManager_->setIndexMode("realtime");

    bool ret = insertDoc_(document, indexDocument, reqlog.timestamp, true);
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

    SCDDoc::const_iterator p = doc.begin();
    for (; p != doc.end(); ++p)
    {
        const string& fieldName = p->first;
        const izenelib::util::UString& propertyValueU = p->second;
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
    IndexerDocument indexDocument, oldIndexDocument;
    docid_t oldId = 0;
    IndexWorker::UpdateType updateType;
    std::string source = "";

    if (!prepareDocument_(scddoc, document, indexDocument, oldIndexDocument, oldId, source, timestamp, updateType, UPDATE_SCD))
    {
        return false;
    }

    if (!indexManager_->isRealTime())
    	indexManager_->setIndexMode("realtime");
    bool ret = updateDoc_(document, indexDocument, oldIndexDocument, reqlog.timestamp, updateType, true);
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
                izenelib::util::UString& propertyValueU = oldvalue.get<izenelib::util::UString>();
                propertyValueU.convertString(oldvalue_str, izenelib::util::UString::UTF_8);
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

uint32_t IndexWorker::getDocNum()
{
    return documentManager_->getNumDocs();
}

uint32_t IndexWorker::getKeyCount(const std::string& property_name)
{
    return indexManager_->getBTreeIndexer()->count(property_name);
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
        const izenelib::util::UString& sourceFieldValue = value.get<izenelib::util::UString>();
        if (sourceFieldValue.empty()) return false;
        sourceFieldValue.convertString(valueStr, izenelib::util::UString::UTF_8);
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

bool IndexWorker::insertOrUpdateSCD_(
        ScdParser& parser,
        SCD_TYPE scdType,
        uint32_t numdoc,
        time_t timestamp)
{
    CREATE_SCOPED_PROFILER (insertOrUpdateSCD, "IndexWorker", "IndexWorker::insertOrUpdateSCD_");

    uint32_t n = 0;
    long lastOffset = 0;
    //for (ScdParser::cached_iterator doc_iter = parser.cbegin(propertyList_);
    //doc_iter != parser.cend(); ++doc_iter, ++n)
    Document document;
    IndexerDocument indexDocument, oldIndexDocument;

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

        SCDDocPtr doc = (*doc_iter);
        docid_t oldId = 0;
        IndexWorker::UpdateType updateType;
        std::string source = "";
        time_t new_timestamp = timestamp;
        document.clear();
        indexDocument.clear();
        oldIndexDocument.clear();
        if (!prepareDocument_(*doc, document, indexDocument, oldIndexDocument, oldId, source, new_timestamp, updateType, scdType))
            continue;

        if (!source.empty())
        {
            ++productSourceCount_[source];
        }

        if (scdType == INSERT_SCD || oldId == 0)
        {
            if (!insertDoc_(document, indexDocument, timestamp))
                continue;
        }
        else
        {
            if (!updateDoc_(document, indexDocument, oldIndexDocument, timestamp, updateType))
                continue;

            ++numUpdatedDocs_;
        }

        // interrupt when closing the process
        boost::this_thread::interruption_point();
    } // end of for loop for all documents
    flushUpdateBuffer_();
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
    std::vector<izenelib::util::UString> rawDocIDList;
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
    for (std::vector<izenelib::util::UString>::iterator iter = rawDocIDList.begin();
            iter != rawDocIDList.end(); ++iter)
    {
        docid_t docId;
        std::string docid_str;
        iter->convertString(docid_str, izenelib::util::UString::UTF_8);
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
            //LOG(WARNING) << "Cannot delete removed Document. docid. " << *iter;
            continue;
        }
        ++indexProgress_.currentFilePos_;

    }

    // interrupt when closing the process
    boost::this_thread::interruption_point();

    searchWorker_->clearSearchCache();
    searchWorker_->clearFilterCache();

    return true;
}

bool IndexWorker::insertDoc_(
        Document& document,
        IndexerDocument& indexDocument,
        time_t timestamp,
        bool immediately)
{
    prepareIndexRTypeProperties_(document.getId(), indexDocument);
    if (hooker_)
    {
        ///TODO compatibility issue:
        if (!hooker_->HookInsert(document, indexDocument, timestamp))
            return false;
    }
    if (immediately)
        return doInsertDoc_(document, indexDocument);

    ///updateBuffer_ is used to change random IO in DocumentManager to sequential IO
    UpdateBufferDataType& updateData = updateBuffer_[document.getId()];
    updateData.get<0>() = INSERT;
    updateData.get<1>().swap(document);
    updateData.get<2>().swap(indexDocument);
    if (updateBuffer_.size() >= UPDATE_BUFFER_CAPACITY)
    {
        flushUpdateBuffer_();
    }
    return true;
}

bool IndexWorker::doInsertDoc_(
        Document& document,
        IndexerDocument& indexDocument)
{
    CREATE_PROFILER(proDocumentIndexing, "IndexWorker", "IndexWorker : InsertDocument")
    CREATE_PROFILER(proIndexing, "IndexWorker", "IndexWorker : indexing")

    START_PROFILER(proDocumentIndexing);
    if (documentManager_->insertDocument(document))
    {
        STOP_PROFILER(proDocumentIndexing);

        START_PROFILER(proIndexing);
        indexManager_->insertDocument(indexDocument);
        STOP_PROFILER(proIndexing);
        indexStatus_.numDocs_ = indexManager_->numDocs();
        return true;
    }
    return false;
}

bool IndexWorker::updateDoc_(
        Document& document,
        IndexerDocument& indexDocument,
        IndexerDocument& oldIndexDocument,
        time_t timestamp,
        IndexWorker::UpdateType updateType,
        bool immediately)
{
    CREATE_SCOPED_PROFILER (proDocumentUpdating, "IndexWorker", "IndexWorker::UpdateDocument");
    if (INSERT == updateType)
        return insertDoc_(document, indexDocument, timestamp, immediately);

    //LOG (INFO) << "Here Down Is Rtype Document ...";
    prepareIndexRTypeProperties_(document.getId(), indexDocument);
    if (hooker_)
    {
        ///Notice: the success of HookUpdate will not affect following update
        hooker_->HookUpdate(document, indexDocument, timestamp);
    }

    if (immediately)
        return doUpdateDoc_(document, indexDocument, oldIndexDocument, updateType);

    ///updateBuffer_ is used to change random IO in DocumentManager to sequential IO
    std::pair<UpdateBufferType::iterator, bool> insertResult =
        updateBuffer_.insert(document.getId(), UpdateBufferDataType());

    UpdateBufferDataType& updateData = insertResult.first.data();
    updateData.get<0>() = updateType;
    updateData.get<1>().swap(document);
    updateData.get<2>().swap(indexDocument);

    // for duplicated DOCIDs in update SCD, only the first instance of
    // oldIndexDocument stores the values before indexing, in order to enable
    // BTreeIndexer to remove these old values properly, we should keep only
    // the first instance of oldIndexDocument
    if (insertResult.second)
    {
        updateData.get<3>().swap(oldIndexDocument);
    }

    if (updateBuffer_.size() >= UPDATE_BUFFER_CAPACITY)
    {
        flushUpdateBuffer_();
    }

    return true;
}

bool IndexWorker::doUpdateDoc_(
        Document& document,
        IndexerDocument& indexDocument,
        IndexerDocument& oldIndexDocument,
        IndexWorker::UpdateType updateType)
{
    switch (updateType)
    {
    case GENERAL:
    {
        uint32_t oldId = indexDocument.getOldId();
        mergeDocument_(oldId, document, indexDocument, true);

        if (!documentManager_->removeDocument(oldId))
        {
            LOG(WARNING) << "document " << oldId << " is already deleted";
        }
        else
        {
            miningTaskService_->EnsureHasDeletedDocDuringMining();
        }
        indexManager_->updateDocument(indexDocument);

        if (!documentManager_->insertDocument(document))
        {
            LOG(ERROR) << "Document Insert Failed in SDB. " << document.property("DOCID");
            return false;
        }
        return true;
    }

    case RTYPE:
    {
        // Store the old property value.
        indexManager_->updateRtypeDocument(oldIndexDocument, indexDocument);
        return true;
    }

    case REPLACE:
    {
        uint32_t oldId = indexDocument.getOldId();
        mergeDocument_(oldId, document, indexDocument, false);

        return documentManager_->updateDocument(document);
    }

    default:
        return false;
    }
}

void IndexWorker::flushUpdateBuffer_()
{
    for (UpdateBufferType::iterator it = updateBuffer_.begin();
            it != updateBuffer_.end(); ++it)
    {
        UpdateBufferDataType& updateData = it->second;
        switch (updateData.get<0>())
        {
        case INSERT:
        {
            if (documentManager_->insertDocument(updateData.get<1>()))
            {
                indexManager_->insertDocument(updateData.get<2>());
                indexStatus_.numDocs_ = indexManager_->numDocs();
            }
            else
                LOG(ERROR) << "Document Insert Failed in SDB. " << updateData.get<1>().property("DOCID");
            break;
        }

        case GENERAL:
        {
            uint32_t oldId = updateData.get<2>().getOldId();
            if (!mergeDocument_(oldId, updateData.get<1>(), updateData.get<2>(), true))
            {
                //updateData.get<0>() = UNKNOWN;
                LOG(INFO) << "doc id: " << it->first << " merger failed in flush general.";
                break;
            }

            if (documentManager_->removeDocument(oldId))
            {
                miningTaskService_->EnsureHasDeletedDocDuringMining();
            }

            indexManager_->updateDocument(updateData.get<2>());

            if (!documentManager_->insertDocument(updateData.get<1>()))
            {
                LOG(ERROR) << "Document Insert Failed in SDB. " << updateData.get<1>().property("DOCID");
            }
            break;
        }

        case REPLACE:
        {
            uint32_t oldId = updateData.get<2>().getOldId();
            if (mergeDocument_(oldId, updateData.get<1>(), updateData.get<2>(), false))
                documentManager_->updateDocument(updateData.get<1>());
            break;
        }

        case RTYPE:
        {
            // Store the old property value.
            indexManager_->updateRtypeDocument(updateData.get<3>(), updateData.get<2>());
            break;
        }

        default:
            break;
        }
    }

    UpdateBufferType().swap(updateBuffer_);
}

bool IndexWorker::deleteDoc_(docid_t docid, time_t timestamp)
{
    CREATE_SCOPED_PROFILER (proDocumentDeleting, "IndexWorker", "IndexWorker::DeleteDocument");

    if (hooker_)
    {
        if (!hooker_->HookDelete(docid, timestamp)) return false;
    }
    if (documentManager_->removeDocument(docid))
    {
        indexManager_->removeDocument(collectionId_, docid);
        ++numDeletedDocs_;
        indexStatus_.numDocs_ = indexManager_->numDocs();
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

bool IndexWorker::prepareDocument_(
        SCDDoc& doc,
        Document& document,
        IndexerDocument& indexDocument,
        IndexerDocument& oldIndexDocument,
        docid_t& oldId,
        std::string& source,
        time_t& timestamp,
        IndexWorker::UpdateType& updateType,
        SCD_TYPE scdType)
{
    CREATE_SCOPED_PROFILER (preparedocument, "IndexWorker", "IndexWorker::prepareDocument_");
    CREATE_PROFILER (pid_date, "IndexWorker", "IndexWorker::prepareDocument__::DATE");

    docid_t docId = 0;
    string fieldStr;
    vector<CharacterOffset> sentenceOffsetList;
    AnalysisInfo analysisInfo;
    if (doc.empty()) return false;
    // the iterator is not const because the p-second value may change
    // due to the maxlen setting

    SCDDoc::iterator p = doc.begin();
    bool dateExistInSCD = false;


    for (; p != doc.end(); p++)
    {
        const std::string& fieldStr = p->first;
        tempPropertyConfig.propertyName_ = fieldStr;
        IndexBundleSchema::iterator iter = bundleConfig_->indexSchema_.find(tempPropertyConfig);
        bool isIndexSchema = (iter != bundleConfig_->indexSchema_.end());
		
        const izenelib::util::UString & propertyValueU = p->second; // preventing copy

        if (!bundleConfig_->productSourceField_.empty()
                && boost::iequals(fieldStr, bundleConfig_->productSourceField_))
        {
            propertyValueU.convertString(source, bundleConfig_->encoding_);
        }

        if (boost::iequals(fieldStr, DOCID) && isIndexSchema)
        {
            izenelib::util::UString::EncodingType encoding = bundleConfig_->encoding_;
            std::string fieldValue;
            propertyValueU.convertString(fieldValue, encoding);
            uint128_t scdDocId = Utilities::md5ToUint128(fieldValue);
            // update
            if (scdType != INSERT_SCD)
            {
                updateType = checkUpdateType_(scdDocId, doc, oldId, docId, scdType);

                if (updateType == INSERT && scdType == RTYPE_SCD)
                    return false;

                if (updateType == RTYPE)
                {
                    if (documentManager_->isDeleted(oldId))
                        return false;

                    prepareIndexRTypeProperties_(oldId, oldIndexDocument);
                }
            }
            else if (!createInsertDocId_(scdDocId, docId))
            {
                //LOG(WARNING) << "failed to create id for SCD DOC " << fieldValue;
                return false;
            }

            document.setId(docId);
            PropertyValue propData(propertyValueU);
            document.property(fieldStr).swap(propData);

            indexDocument.setOldId(oldId);
            indexDocument.setDocId(docId, collectionId_);

            if (oldId && oldId != docId)
                documentManager_->moveRTypeValues(oldId, docId);
        }
        else if (boost::iequals(fieldStr, dateProperty_.getName()))
        {
            /// format <DATE>20091009163011
            START_PROFILER(pid_date);
            dateExistInSCD = true;
            izenelib::util::UString dateStr;
            timestamp = Utilities::createTimeStampInSeconds(propertyValueU, bundleConfig_->encoding_, dateStr);
            boost::shared_ptr<NumericPropertyTableBase>& datePropertyTable = documentManager_->getNumericPropertyTable(dateProperty_.getName());
            if (datePropertyTable)
            {
                datePropertyTable->setInt64Value(docId, timestamp);
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
                        izenelib::util::UString::EncodingType encoding = bundleConfig_->encoding_;
                        std::string fieldValue;
                        propertyValueU.convertString(fieldValue, encoding);
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

                                if (!makeSentenceBlocks_(propertyValueU, iter->getDisplayLength(),
                                        numOfSummary, sentenceOffsetList))
                                {
                                    LOG(ERROR) << "Make Sentence Blocks Failes ";
                                }
                                PropertyValue propData(sentenceOffsetList);
                                document.property(fieldStr + ".blocks").swap(propData);
                            }
                        }
                    }
                    prepareIndexDocumentStringProperty_(docId, *p, iter, indexDocument);
                }
                break;
            case SUBDOC_PROPERTY_TYPE:
                {
                    PropertyValue propData(propertyValueU);
                    document.property(fieldStr).swap(propData);
                    prepareIndexDocumentStringProperty_(docId, *p, iter, indexDocument);
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
                    izenelib::util::UString::EncodingType encoding = bundleConfig_->encoding_;
                    std::string fieldValue;
                    propertyValueU.convertString(fieldValue, encoding);
                    numericPropertyTable->setStringValue(docId, fieldValue);
                }
                else
                {
                    PropertyValue propData(propertyValueU);
                    document.property(fieldStr).swap(propData);
                    prepareIndexDocumentNumericProperty_(docId, p->second, iter, indexDocument);
                }
                if (updateType == RTYPE)
                    documentManager_->RtypeDocidPros_.insert(fieldStr);
                break;

            case DATETIME_PROPERTY_TYPE:
                if (iter->getIsFilter() && !iter->getIsMultiValue())
                {
                    izenelib::util::UString dateStr;
                    time_t ts = Utilities::createTimeStampInSeconds(propertyValueU, bundleConfig_->encoding_, dateStr);
                    boost::shared_ptr<NumericPropertyTableBase>& datePropertyTable = documentManager_->getNumericPropertyTable(iter->getName());
                    datePropertyTable->setInt64Value(docId, ts);
                }
                else
                {
                    PropertyValue propData(propertyValueU);
                    document.property(fieldStr).swap(propData);
                    prepareIndexDocumentNumericProperty_(docId, p->second, iter, indexDocument);
                }
                if (updateType == RTYPE)
                    documentManager_->RtypeDocidPros_.insert(fieldStr);
                break;

            default:
                break;
            }

        }
    }

    ///Processing date
    IndexerPropertyConfig indexerPropertyConfig;
    indexerPropertyConfig.setPropertyId(dateProperty_.getPropertyId());
    indexerPropertyConfig.setName(dateProperty_.getName());
    indexerPropertyConfig.setIsIndex(true);
    indexerPropertyConfig.setIsFilter(true);
    indexerPropertyConfig.setIsAnalyzed(false);
    indexerPropertyConfig.setIsMultiValue(false);

    if (dateExistInSCD)
    {
        timestamp = -1;
    }
    else
    {
        time_t tm = timestamp / 1000000;
        boost::shared_ptr<NumericPropertyTableBase>& numericPropertyTable
            = documentManager_->getNumericPropertyTable(dateProperty_.getName());
        numericPropertyTable->setInt64Value(docId, tm);
    }

    return true;
}

bool IndexWorker::mergeDocument_(
        docid_t oldId,
        Document& doc,
        IndexerDocument& indexDocument,
        bool generateIndexDoc)
{
    ///We need to retrieve old document, and then merge them with new document data
    docid_t newId = doc.getId();
    Document oldDoc;
    if (!documentManager_->getDocument(oldId, oldDoc, true))
    {
        return false;
    }
    oldDoc.copyPropertiesFromDocument(doc);
    for (Document::property_iterator it = oldDoc.propertyBegin(); it != oldDoc.propertyEnd(); ++it)
    {
        if (!doc.hasProperty(it->first) && generateIndexDoc)
        {
            ///Properties that only exist within old doc, while not in new doc
            ///Require to prepare for IndexDocument
            tempPropertyConfig.propertyName_ = it->first;
            IndexBundleSchema::iterator iter = bundleConfig_->indexSchema_.find(tempPropertyConfig);
            if (iter != bundleConfig_->indexSchema_.end())
            {
                const izenelib::util::UString& propValue = oldDoc.property(it->first).get<izenelib::util::UString>();
                if (propValue.empty()) continue;
                prepareIndexDocumentProperty_(newId, std::make_pair(it->first, propValue), iter, indexDocument);
            }
        }
    }

    doc.swap(oldDoc);
    doc.setId(newId);
    return true;
}

bool IndexWorker::prepareIndexDocument_(
        docid_t oldId,
        time_t timestamp,
        const Document& document,
        IndexerDocument& indexDocument)
{
    CREATE_SCOPED_PROFILER (preparedocument, "IndexWorker", "IndexWorker::prepareIndexDocument_");

    docid_t docId = document.getId();//new id;
    AnalysisInfo analysisInfo;
    typedef Document::property_const_iterator document_iterator;
    document_iterator p;
    // the iterator is not const because the p-second value may change
    // due to the maxlen setting
    for (p = document.propertyBegin(); p != document.propertyEnd(); ++p)
    {
        const string& fieldStr = p->first;

        tempPropertyConfig.propertyName_ = fieldStr;
        IndexBundleSchema::iterator iter = bundleConfig_->indexSchema_.find(tempPropertyConfig);

        if (iter == bundleConfig_->indexSchema_.end())
            continue;

        const izenelib::util::UString& propertyValueU = p->second.get<izenelib::util::UString>();

        if (boost::iequals(fieldStr, DOCID))
        {
            indexDocument.setOldId(oldId);
            indexDocument.setDocId(docId, collectionId_);
        }
        else
        {
            prepareIndexDocumentProperty_(docId, std::make_pair(fieldStr, propertyValueU), iter, indexDocument);
        }
    }

    documentManager_->moveRTypeValues(oldId, docId);

    return true;
}

bool IndexWorker::prepareIndexDocumentProperty_(
        docid_t docId,
        const FieldPair& p,
        IndexBundleSchema::iterator iter,
        IndexerDocument& indexDocument)
{
    CREATE_SCOPED_PROFILER (prepareIndexProperty, "IndexWorker", "IndexWorker::prepareIndexDocumentProperty_");
    // the iterator is not const because the p-second value may change
    // due to the maxlen setting
    if (iter->getType() == STRING_PROPERTY_TYPE)
        return prepareIndexDocumentStringProperty_(docId, p, iter, indexDocument);
    else
        return prepareIndexDocumentNumericProperty_(docId, p.second, iter, indexDocument);
}

bool IndexWorker::prepareIndexDocumentStringProperty_(
        docid_t docId,
        const FieldPair& p,
        IndexBundleSchema::iterator iter,
        IndexerDocument& indexDocument)
{
    CREATE_SCOPED_PROFILER (prepareIndexStringProperty, "IndexWorker", "IndexWorker::prepareIndexDocumentStringProperty_");
    izenelib::util::UString::EncodingType encoding = bundleConfig_->encoding_;
    const string& fieldStr = p.first;
    const izenelib::util::UString & propertyValueU = p.second;

    IndexerPropertyConfig indexerPropertyConfig;
    indexerPropertyConfig.setPropertyId(iter->getPropertyId());
    indexerPropertyConfig.setName(iter->getName());
    indexerPropertyConfig.setIsIndex(iter->isIndex());
    indexerPropertyConfig.setIsAnalyzed(iter->isAnalyzed());
    indexerPropertyConfig.setIsFilter(iter->getIsFilter());
    indexerPropertyConfig.setIsMultiValue(iter->getIsMultiValue());
    indexerPropertyConfig.setIsStoreDocLen(iter->getIsStoreDocLen());

    if (!propertyValueU.empty())
    {
        ///process for properties that requires forward index to be created
        if (iter->isIndex())
        {
            const AnalysisInfo& analysisInfo = iter->getAnalysisInfo();
            if (analysisInfo.analyzerId_.empty())
            {
                if (iter->getIsFilter() && iter->getIsMultiValue())
                {
                    MultiValuePropertyType props;
                    split_string(propertyValueU, props, encoding, ',');
                    indexDocument.insertProperty(indexerPropertyConfig, props);
                }
                else
                    indexDocument.insertProperty(indexerPropertyConfig, propertyValueU);
            }
            else
            {
                if (!makeForwardIndex_(docId, propertyValueU, fieldStr, iter->getPropertyId(), analysisInfo))
                {
                    LOG(ERROR) << "Forward Indexing Failed Error Line : " << __LINE__;
                    return false;
                }
                if (iter->getIsFilter())
                {
                    if (iter->getIsMultiValue())
                    {
                        MultiValuePropertyType props;
                        split_string(propertyValueU, props, encoding,',');

                        MultiValueIndexPropertyType indexData =
                            std::make_pair(laInputs_[iter->getPropertyId()], props);
                        indexDocument.insertProperty(indexerPropertyConfig, indexData);
                    }
                    else
                    {
                        IndexPropertyType indexData = std::make_pair(
                                laInputs_[iter->getPropertyId()], propertyValueU);
                        indexDocument.insertProperty(indexerPropertyConfig, indexData);
                    }
                }
                else
                    indexDocument.insertProperty(
                            indexerPropertyConfig, laInputs_[iter->getPropertyId()]);

                // For alias indexing
                config_tool::PROPERTY_ALIAS_MAP_T::iterator mapIter =
                    propertyAliasMap_.find(iter->getName());
                if (mapIter != propertyAliasMap_.end()) // if there's alias property
                {
                    std::vector<PropertyConfig>::iterator vecIter =
                        mapIter->second.begin();
                    for (; vecIter != mapIter->second.end(); vecIter++)
                    {
                        AnalysisInfo aliasAnalysisInfo =
                            vecIter->getAnalysisInfo();
                        if (!makeForwardIndex_(
                                    docId,
                                    propertyValueU,
                                    fieldStr,
                                    vecIter->getPropertyId(),
                                    aliasAnalysisInfo))
                        {
                            LOG(ERROR) << "Forward Indexing Failed Error Line : " << __LINE__;
                            return false;
                        }
                        IndexerPropertyConfig aliasIndexerPropertyConfig(
                                vecIter->getPropertyId(),
                                vecIter->getName(),
                                vecIter->isIndex(),
                                vecIter->isAnalyzed());
                        aliasIndexerPropertyConfig.setIsFilter(vecIter->getIsFilter());
                        aliasIndexerPropertyConfig.setIsMultiValue(vecIter->getIsMultiValue());
                        aliasIndexerPropertyConfig.setIsStoreDocLen(vecIter->getIsStoreDocLen());
                        indexDocument.insertProperty(
                                aliasIndexerPropertyConfig, laInputs_[vecIter->getPropertyId()]);
                    } // end - for
                } // end - if (mapIter != end())

            }
        }
        // insert property name and value for other properties that is not DOCID and neither required to be indexed
        else
        {
            //other extra properties that need not be in index manager
            indexDocument.insertProperty(indexerPropertyConfig, propertyValueU);
        }
    }

    return true;
}

bool IndexWorker::prepareIndexRTypeProperties_(
        docid_t docId,
        IndexerDocument& indexDocument)
{
    CREATE_SCOPED_PROFILER (prepareIndexRTypeProperties, "IndexWorker", "IndexWorker::prepareIndexDocumentRTypeProperties_");
    CREATE_PROFILER (pid_int32, "IndexWorker", "IndexWorker::prepareIndexDocument_::INT32");
    CREATE_PROFILER (pid_float, "IndexWorker", "IndexWorker::prepareIndexDocument_::FLOAT");
    CREATE_PROFILER (pid_int64, "IndexWorker", "IndexWorker::prepareIndexDocument_::INT64");

    IndexerPropertyConfig indexerPropertyConfig;

    DocumentManager::RTypeStringPropTableMap& rtype_string_proptable = documentManager_->getRTypeStringPropTableMap();
    for (DocumentManager::RTypeStringPropTableMap::const_iterator rtype_it = rtype_string_proptable.begin();
            rtype_it != rtype_string_proptable.end(); ++rtype_it)
    {
        tempPropertyConfig.propertyName_ = rtype_it->first;
        IndexBundleSchema::iterator index_it = bundleConfig_->indexSchema_.find(tempPropertyConfig);
        if (index_it == bundleConfig_->indexSchema_.end())
            continue;
        FieldPair prop_pair;
        std::string s_propvalue;
        rtype_it->second->getRTypeString(docId, s_propvalue);
        prop_pair.first = rtype_it->first;
        prop_pair.second = UString(s_propvalue, bundleConfig_->encoding_);

        prepareIndexDocumentStringProperty_(docId, prop_pair, index_it, indexDocument);
    }

    DocumentManager::NumericPropertyTableMap& numericPropertyTables = documentManager_->getNumericPropertyTableMap();
    bool ret = false;
    for (DocumentManager::NumericPropertyTableMap::const_iterator it = numericPropertyTables.begin();
            it != numericPropertyTables.end(); ++it)
    {
        tempPropertyConfig.propertyName_ = it->first;
        IndexBundleSchema::iterator iter = bundleConfig_->indexSchema_.find(tempPropertyConfig);

        if (iter == bundleConfig_->indexSchema_.end())
            continue;

        indexerPropertyConfig.setPropertyId(iter->getPropertyId());
        indexerPropertyConfig.setName(iter->getName());
        indexerPropertyConfig.setIsIndex(iter->isIndex());
        indexerPropertyConfig.setIsAnalyzed(iter->isAnalyzed());
        indexerPropertyConfig.setIsFilter(iter->getIsFilter());
        indexerPropertyConfig.setIsMultiValue(iter->getIsMultiValue());
        indexerPropertyConfig.setIsStoreDocLen(iter->getIsStoreDocLen());

        switch (iter->getType())
        {
        case INT32_PROPERTY_TYPE:
        case INT8_PROPERTY_TYPE:
        case INT16_PROPERTY_TYPE:
            START_PROFILER(pid_int32)
            if (iter->getIsRange())
            {
                std::pair<int32_t, int32_t> value;
                NumericRangePropertyTable<int32_t>* numericPropertyTable = static_cast<NumericRangePropertyTable<int32_t> *>(it->second.get());
                if (!numericPropertyTable->getValue(docId, value))
                    break;

                if (value.first == value.second)
                {
                    indexDocument.insertProperty(indexerPropertyConfig, value.first);
                }
                else
                {
                    indexerPropertyConfig.setIsMultiValue(true);
                    MultiValuePropertyType multiProps;
                    multiProps.push_back(value.first);
                    multiProps.push_back(value.second);
                    indexDocument.insertProperty(indexerPropertyConfig, multiProps);
                }
            }
            else
            {
                int32_t value;
                if (!it->second->getInt32Value(docId, value))
                    break;

                indexDocument.insertProperty(indexerPropertyConfig, value);
            }
            STOP_PROFILER(pid_int32)
            break;

        case FLOAT_PROPERTY_TYPE:
            START_PROFILER(pid_float)
            if (iter->getIsRange())
            {
                std::pair<float, float> value;
                NumericRangePropertyTable<float>* numericPropertyTable = static_cast<NumericRangePropertyTable<float> *>(it->second.get());
                if (!numericPropertyTable->getValue(docId, value))
                    break;

                if (value.first == value.second)
                {
                    indexDocument.insertProperty(indexerPropertyConfig, value.first);
                }
                else
                {
                    indexerPropertyConfig.setIsMultiValue(true);
                    MultiValuePropertyType multiProps;
                    multiProps.push_back(value.first);
                    multiProps.push_back(value.second);
                    indexDocument.insertProperty(indexerPropertyConfig, multiProps);
                }
            }
            else
            {
                float value;
                if (!it->second->getFloatValue(docId, value))
                    break;

                indexDocument.insertProperty(indexerPropertyConfig, value);
            }
            STOP_PROFILER(pid_float)
            break;

        case DATETIME_PROPERTY_TYPE:
        case INT64_PROPERTY_TYPE:
            START_PROFILER(pid_int64)
            if (iter->getIsRange())
            {
                std::pair<int64_t, int64_t> value;
                NumericRangePropertyTable<int64_t>* numericPropertyTable = static_cast<NumericRangePropertyTable<int64_t> *>(it->second.get());
                if (!numericPropertyTable->getValue(docId, value))
                    break;

                if (value.first == value.second)
                {
                    indexDocument.insertProperty(indexerPropertyConfig, value.first);
                }
                else
                {
                    indexerPropertyConfig.setIsMultiValue(true);
                    MultiValuePropertyType multiProps;
                    multiProps.push_back(value.first);
                    multiProps.push_back(value.second);
                    indexDocument.insertProperty(indexerPropertyConfig, multiProps);
                }
            }
            else
            {
                int64_t value;
                if (!it->second->getInt64Value(docId, value))
                    break;

                indexDocument.insertProperty(indexerPropertyConfig, value);
            }
            STOP_PROFILER(pid_int64)
            break;

        default:
            break;
        }
        ret = true;
    }

    return ret;
}

bool IndexWorker::prepareIndexDocumentNumericProperty_(
        docid_t docId,
        const izenelib::util::UString& propertyValueU,
        IndexBundleSchema::iterator iter,
        IndexerDocument& indexDocument)
{
    CREATE_SCOPED_PROFILER (prepareIndexNumericProperty, "IndexWorker", "IndexWorker::prepareIndexDocumentNumericProperty_");
    CREATE_PROFILER (pid_int32, "IndexWorker", "IndexWorker::prepareIndexDocument_::INT32");
    CREATE_PROFILER (pid_float, "IndexWorker", "IndexWorker::prepareIndexDocument_::FLOAT");
    CREATE_PROFILER (pid_datetime, "IndexWorker", "IndexWorker::prepareIndexDocument_::DATETIME");
    CREATE_PROFILER (pid_int64, "IndexWorker", "IndexWorker::prepareIndexDocument_::INT64");

    if (!iter->isIndex()) return false;
    izenelib::util::UString::EncodingType encoding = bundleConfig_->encoding_;

    IndexerPropertyConfig indexerPropertyConfig;
    indexerPropertyConfig.setPropertyId(iter->getPropertyId());
    indexerPropertyConfig.setName(iter->getName());
    indexerPropertyConfig.setIsIndex(iter->isIndex());
    indexerPropertyConfig.setIsAnalyzed(iter->isAnalyzed());
    indexerPropertyConfig.setIsFilter(iter->getIsFilter());
    indexerPropertyConfig.setIsMultiValue(iter->getIsMultiValue());
    indexerPropertyConfig.setIsStoreDocLen(iter->getIsStoreDocLen());

    switch (iter->getType())
    {
    case INT32_PROPERTY_TYPE:
    case INT8_PROPERTY_TYPE:
    case INT16_PROPERTY_TYPE:
    {
        START_PROFILER(pid_int32);
        if (iter->getIsMultiValue())
        {
            MultiValuePropertyType props;
            split_int32(propertyValueU,props, encoding,",;");
            indexDocument.insertProperty(indexerPropertyConfig, props);
        }
        else
        {
            std::string str;
            propertyValueU.convertString(str, encoding);
            int32_t value = 0;
            try
            {
                value = boost::lexical_cast<int32_t>(str);
                indexDocument.insertProperty(indexerPropertyConfig, value);
            }
            catch (const boost::bad_lexical_cast &)
            {
                MultiValuePropertyType multiProps;
                if (checkSeparatorType_(propertyValueU, encoding, '-'))
                {
                    split_int32(propertyValueU, multiProps, encoding,"-");
                    indexerPropertyConfig.setIsMultiValue(true);
                    indexDocument.insertProperty(indexerPropertyConfig, multiProps);
                }
                else if (checkSeparatorType_(propertyValueU, encoding, '~'))
                {
                    split_int32(propertyValueU, multiProps, encoding,"~");
                    indexerPropertyConfig.setIsMultiValue(true);
                    indexDocument.insertProperty(indexerPropertyConfig, multiProps);
                }
                else if (checkSeparatorType_(propertyValueU, encoding, ','))
                {
                    split_int32(propertyValueU, multiProps, encoding,",");
                    indexerPropertyConfig.setIsMultiValue(true);
                    indexDocument.insertProperty(indexerPropertyConfig, multiProps);
                }
                else
                {
                    try
                    {
                        value = (int32_t)(boost::lexical_cast<float>(str));
                        indexDocument.insertProperty(indexerPropertyConfig, value);
                    }
                    catch (const boost::bad_lexical_cast &)
                    {
                        //LOG(ERROR) << "Wrong format of number value. DocId " << docId <<" Property "<<fieldStr<< " Value" << str;
                    }
                }
            }
        }
        STOP_PROFILER(pid_int32);
        break;
    }
    case FLOAT_PROPERTY_TYPE:
    {
        START_PROFILER(pid_float);
        if (iter->getIsMultiValue())
        {
            MultiValuePropertyType props;
            split_float(propertyValueU,props, encoding,",;");
            indexDocument.insertProperty(indexerPropertyConfig, props);
        }
        else
        {
            std::string str;
            propertyValueU.convertString(str, encoding);
            float value = 0;
            try
            {
                value = boost::lexical_cast<float>(str);
                indexDocument.insertProperty(indexerPropertyConfig, value);
            }
            catch (const boost::bad_lexical_cast &)
            {
                MultiValuePropertyType multiProps;
                if (checkSeparatorType_(propertyValueU, encoding, '-'))
                    split_float(propertyValueU, multiProps, encoding,"-");
                else if (checkSeparatorType_(propertyValueU, encoding, '~'))
                    split_float(propertyValueU, multiProps, encoding,"~");
                else if (checkSeparatorType_(propertyValueU, encoding, ','))
                    split_float(propertyValueU, multiProps, encoding,",");
                indexerPropertyConfig.setIsMultiValue(true);
                indexDocument.insertProperty(indexerPropertyConfig, multiProps);
            }
        }
        STOP_PROFILER(pid_float);
        break;
    }
    case DATETIME_PROPERTY_TYPE:
    {
        START_PROFILER(pid_datetime);
        if (iter->getIsMultiValue())
        {
            MultiValuePropertyType props;
            split_datetime(propertyValueU,props, encoding,",;");
            indexDocument.insertProperty(indexerPropertyConfig, props);
        }
        else
        {
            time_t timestamp = Utilities::createTimeStampInSeconds(propertyValueU);
            indexDocument.insertProperty(indexerPropertyConfig, timestamp);
        }
        STOP_PROFILER(pid_datetime);
        break;
    }
    case INT64_PROPERTY_TYPE:
    {
        START_PROFILER(pid_int64);
        if (iter->getIsMultiValue())
        {
            MultiValuePropertyType props;
            split_int64(propertyValueU,props, encoding,",;");
            indexDocument.insertProperty(indexerPropertyConfig, props);
        }
        else
        {
            std::string str;
            propertyValueU.convertString(str, encoding);
            int64_t value = 0;
            try
            {
                value = boost::lexical_cast<int64_t>(str);
                indexDocument.insertProperty(indexerPropertyConfig, value);
            }
            catch (const boost::bad_lexical_cast &)
            {
                MultiValuePropertyType multiProps;
                if (checkSeparatorType_(propertyValueU, encoding, '-'))
                {
                    split_int64(propertyValueU, multiProps, encoding,"-");
                    indexerPropertyConfig.setIsMultiValue(true);
                    indexDocument.insertProperty(indexerPropertyConfig, multiProps);
                }
                else if (checkSeparatorType_(propertyValueU, encoding, '~'))
                {
                    split_int64(propertyValueU, multiProps, encoding,"~");
                    indexerPropertyConfig.setIsMultiValue(true);
                    indexDocument.insertProperty(indexerPropertyConfig, multiProps);
                }
                else if (checkSeparatorType_(propertyValueU, encoding, ','))
                {
                    split_int64(propertyValueU, multiProps, encoding,",");
                    indexerPropertyConfig.setIsMultiValue(true);
                    indexDocument.insertProperty(indexerPropertyConfig, multiProps);
                }
                else
                {
                    try
                    {
                        value = (int64_t)(boost::lexical_cast<float>(str));
                        indexDocument.insertProperty(indexerPropertyConfig, value);
                    }
                    catch (const boost::bad_lexical_cast &)
                    {
                        //LOG(ERROR) << "Wrong format of number value. DocId " << docId <<" Property "<<fieldStr<< " Value" << str;
                    }
                }
            }
        }
        STOP_PROFILER(pid_int64);
        break;
    }
    default:
        break;
    }
    return true;
}

bool IndexWorker::checkSeparatorType_(
        const izenelib::util::UString& propertyValueStr,
        izenelib::util::UString::EncodingType encoding,
        char separator)
{
    izenelib::util::UString tmpStr(propertyValueStr);
    izenelib::util::UString sep(" ", encoding);
    sep[0] = separator;
    size_t n = 0;
    n = tmpStr.find(sep,0);
    if (n != izenelib::util::UString::npos)
        return true;
    return false;
}

IndexWorker::UpdateType IndexWorker::checkUpdateType_(
        const uint128_t& scdDocId,
        SCDDoc& doc,
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

    SCDDoc::iterator p = doc.begin();
    for (; p != doc.end(); ++p)
    {
        const string& fieldName = p->first;
        const izenelib::util::UString& propertyValueU = p->second;
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
        const izenelib::util::UString & text,
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

/// @desc Make a forward index of a given text.
/// You can specify an Language Analysis option through AnalysisInfo parameter.
/// You have to get a proper AnalysisInfo value from the configuration. (Currently not implemented.)
bool IndexWorker::makeForwardIndex_(
        docid_t docId,
        const izenelib::util::UString& text,
        const std::string& propertyName,
        unsigned int propertyId,
        const AnalysisInfo& analysisInfo)
{
    CREATE_SCOPED_PROFILER(proTermExtracting, "IndexWorker", "Analyzer overhead");

//    la::TermIdList termIdList;
    boost::shared_ptr<LAInput>& laInput = laInputs_[propertyId];
    laInput.reset(new LAInput);
    laInput->setDocId(docId);

    // Remove the spaces between two Chinese Characters
//    izenelib::util::UString refinedText;
//    la::removeRedundantSpaces(text, refinedText);
//    if (!laManager_->getTermList(refinedText, analysisInfo, true, termList, true))
    la::MultilangGranularity indexingLevel = bundleConfig_->indexMultilangGranularity_;
    if (indexingLevel == la::SENTENCE_LEVEL)
    {
        if (bundleConfig_->bIndexUnigramProperty_)
        {
            if (propertyName.find("_unigram") != std::string::npos)
                indexingLevel = la::FIELD_LEVEL;  /// for unigram property, we do not need sentence level indexing
        }
    }

    if (!laManager_->getTermIdList(idManager_.get(), text, analysisInfo, *laInput, indexingLevel))
        return false;

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

bool IndexWorker::requireBackup_(size_t currTotalScdSizeInMB)
{
    return false;
}

bool IndexWorker::backup_()
{
    // not copy, always returns true
    return true;
}

bool IndexWorker::recoverSCD_()
{
    const boost::shared_ptr<Directory>& currentDir
    = directoryRotator_.currentDirectory();
    const boost::shared_ptr<Directory>& next
    = directoryRotator_.nextDirectory();

    if (!(next && currentDir->name() != next->name()))
        return false;

    std::ifstream scdLogInput;
    scdLogInput.open(currentDir->scdLogString().c_str());
    std::istream_iterator<std::string> begin(scdLogInput);
    std::istream_iterator<std::string> end;
    boost::unordered_set<std::string> existingSCDs;
    for (std::istream_iterator<std::string> it = begin; it != end; ++it)
    {
        std::cout << *it << "@@"<<std::endl;
        existingSCDs.insert(*it);
    }
    if (existingSCDs.empty()) return false;
    bfs::path scdBkDir = bfs::path(bundleConfig_->indexSCDPath()) / SCD_BACKUP_DIR;

    try
    {
        if (!bfs::is_directory(scdBkDir))
        {
            return false;
        }
    }
    catch (bfs::filesystem_error& e)
    {
        return false;
    }

    // search the directory for files
    static const bfs::directory_iterator kItrEnd;
    bfs::path scdIndexDir = bfs::path(bundleConfig_->indexSCDPath());

    for (bfs::directory_iterator itr(scdBkDir); itr != kItrEnd; ++itr)
    {
        if (bfs::is_regular_file(itr->status()))
        {
            std::string fileName = itr->path().filename().string();
            if (existingSCDs.find(fileName) == existingSCDs.end())
            {
                try
                {
                    bfs::rename(*itr, scdIndexDir / bfs::path(fileName));
                }
                catch (bfs::filesystem_error& e)
                {
                    LOG(WARNING) << "exception in recovering file " << fileName << ": " << e.what();
                }
            }
        }
    }

    return true;
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
        scddoc[insertto].second.assign(asString(it->second), izenelib::util::UString::UTF_8);
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
        const izenelib::util::UString& propValue = document.property(it->first).get<izenelib::util::UString>();
        if (boost::iequals(propertyName, DOCID))
        {
            insertto = 0;
            --propertyId;
        }
        scddoc[insertto].first.assign(it->first);
        scddoc[insertto].second.assign(propValue);
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
