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

// xxx
#include <bundles/index/IndexBundleConfiguration.h>
#include <bundles/mining/MiningTaskService.h>
#include <bundles/recommend/RecommendTaskService.h>

#include <util/profiler/ProfilerGroup.h>

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
}

IndexWorker::~IndexWorker()
{
    delete scd_writer_;
}

void IndexWorker::index(unsigned int numdoc, bool& result)
{
    task_type task = boost::bind(&IndexWorker::buildCollection, this, numdoc);
    JobScheduler::get()->addTask(task, bundleConfig_->collectionName_);
    result = true;
}

bool IndexWorker::reindex(boost::shared_ptr<DocumentManager>& documentManager)
{
    //task_type task = boost::bind(&IndexWorker::rebuildCollection, this, documentManager);
    //JobScheduler::get()->addTask(task, bundleConfig_->collectionName_);
    rebuildCollection(documentManager);
    return true;
}

bool IndexWorker::buildCollection(unsigned int numdoc)
{
    size_t currTotalSCDSize = getTotalScdSize_();
    ///If current directory is the one rotated from the backup directory,
    ///there should exist some missed SCDs since the last backup time,
    ///so we move those SCDs from backup directory, so that these data
    ///could be recovered through indexing
    recoverSCD_();

    string scdPath = bundleConfig_->indexSCDPath();
    Status::Guard statusGuard(indexStatus_);
    CREATE_PROFILER(buildIndex, "Index:SIAProcess", "Indexer : buildIndex")

    START_PROFILER(buildIndex);

    LOG(INFO) << "start BuildCollection";

    izenelib::util::ClockTimer timer;

    //flush all writing SCDs
    scd_writer_->Flush();

    indexProgress_.reset();

    ScdParser parser(bundleConfig_->encoding_);

    // saves the name of the scd files in the path
    vector<string> scdList;
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

    indexProgress_.totalFileNum = scdList.size();

    if (indexProgress_.totalFileNum == 0)
    {
        LOG(WARNING) << "SCD Files do not exist. Path " << scdPath;
        if (miningTaskService_)
        {
            miningTaskService_->DoContinue();
        }
        return false;
    }

    indexProgress_.currentFileIdx = 1;

    //sort scdList
    sort(scdList.begin(), scdList.end(), ScdParser::compareSCD);

    //here, try to set the index mode(default[batch] or realtime)
    //The threshold is set to the scd_file_size/exist_doc_num, if smaller or equal than this threshold then realtime mode will turn on.
    //when the scd file size(M) larger than max_realtime_msize, the default mode will turn on while ignore the threshold above.
    long max_realtime_msize = 50L; //set to 50M
    double threshold = 50.0/500000.0;
    IndexModeSelector index_mode_selector(indexManager_, threshold, max_realtime_msize);
    index_mode_selector.TrySetIndexMode(indexProgress_.totalFileSize_);

    if(!indexManager_->isRealTime())
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

        LOG(INFO) << "SCD Files in Path processed in given order. Path " << scdPath;
        vector<string>::iterator scd_it;
        for (scd_it = scdList.begin(); scd_it != scdList.end(); ++scd_it)
            LOG(INFO) << "SCD File " << bfs::path(*scd_it).stem();

        try
        {
            // loops the list of SCD files that belongs to this collection
            long proccessedFileSize = 0;
            idManager_->warmUp();
            for (scd_it = scdList.begin(); scd_it != scdList.end(); scd_it++)
            {
                size_t pos = scd_it ->rfind("/") + 1;
                string filename = scd_it ->substr(pos);
                indexProgress_.currentFileName = filename;
                indexProgress_.currentFilePos_ = 0;

                LOG(INFO) << "Processing SCD file. " << bfs::path(*scd_it).stem();

                switch (parser.checkSCDType(*scd_it))
                {
                case INSERT_SCD:
                {
                    if (!doBuildCollection_(*scd_it, 1, numdoc))
                    {
                        //continue;
                    }
                    LOG(INFO) << "Indexing Finished";

                }
                break;
                case DELETE_SCD:
                {
                    if (documentManager_->getMaxDocId() > 0)
                    {
                        doBuildCollection_(*scd_it, 3, 0);
                        LOG(INFO) << "Delete Finished";
                    }
                    else
                    {
                        LOG(WARNING) << "Indexed documents do not exist. File " << bfs::path(*scd_it).stem();
                    }
                }
                break;
                case UPDATE_SCD:
                {
                    doBuildCollection_(*scd_it, 2, 0);
                    LOG(INFO) << "Update Finished";
                }
                break;
                default:
                    break;
                }
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
            idManager_->coolDown();
            return false;
        }

        bfs::path bkDir = bfs::path(scdPath) / SCD_BACKUP_DIR;
        bfs::create_directory(bkDir);
        LOG(INFO) << "moving " << scdList.size() << " SCD files to directory " << bkDir;
        const boost::shared_ptr<Directory>& currentDir = directoryRotator_.currentDirectory();

        for (scd_it = scdList.begin(); scd_it != scdList.end(); ++scd_it)
        {
            try
            {
                bfs::rename(*scd_it, bkDir / bfs::path(*scd_it).filename());
                currentDir->appendSCD(bfs::path(*scd_it).filename().string());
            }
            catch (bfs::filesystem_error& e)
            {
                LOG(WARNING) << "exception in rename file " << *scd_it << ": " << e.what();
            }
        }

    }///set cookie as true here
    try{
#ifdef __x86_64
        if (bundleConfig_->isTrieWildcard())
        {
            idManager_->startWildcardProcess();
            idManager_->joinWildcardProcess();
        }
#endif

        if (hooker_)
        {
            if (!hooker_->FinishHook())
            {
                std::cout<<"[IndexWorker] Hooker Finish failed."<<std::endl;
                idManager_->coolDown();
                return false;
            }
            std::cout<<"[IndexWorker] Hooker Finished."<<std::endl;
        }

        if (miningTaskService_)
        {
            indexManager_->pauseMerge();
            miningTaskService_->DoMiningCollection();
            indexManager_->resumeMerge();
        }

        idManager_->coolDown();
    }
    catch (std::exception& e)
    {
        LOG(WARNING) << "exception in mining: " << e.what();
        indexProgress_.getIndexingStatus(indexStatus_);
        indexProgress_.reset();
        idManager_->coolDown();
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

bool IndexWorker::rebuildCollection(boost::shared_ptr<DocumentManager>& documentManager)
{
    LOG(INFO) << "start BuildCollection";

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
    idManager_->warmUp();
    for (curDocId = minDocId; curDocId <= maxDocId; curDocId++)
    {
        if (documentManager->isDeleted(curDocId))
        {
            //LOG(INFO) << "skip deleted docid: " << curDocId;
            continue;
        }

        Document document;
        bool b = documentManager->getDocument(curDocId, document);
        if(!b) continue;

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
            //LOG(WARNING) << "Failed to create new docid for: " << curDocId;
            continue;
        }

        IndexerDocument indexDocument;
        time_t timestamp = -1;
        prepareIndexDocument_(oldId, timestamp, document, indexDocument);

        timestamp = Utilities::createTimeStamp();
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
    LOG(INFO) << "inserted doc number: " << insertedCount << ", total: " << maxDocId;
    LOG(INFO) << "Indexing Finished";

    documentManager_->flush();
    idManager_->flush();
    indexManager_->flush();

#ifdef __x86_64
    if (bundleConfig_->isTrieWildcard())
    {
        idManager_->startWildcardProcess();
        idManager_->joinWildcardProcess();
    }
#endif

    if (miningTaskService_)
    {
        indexManager_->pauseMerge();
        miningTaskService_->DoMiningCollection();
        indexManager_->resumeMerge();
    }

    idManager_->coolDown();

    LOG(INFO) << "End BuildCollection: ";
    LOG(INFO) << "time elapsed:" << timer.elapsed() <<"seconds";

    return true;
}

bool IndexWorker::optimizeIndex()
{
    if (!backup_())
        return false;

    DirectoryGuard dirGuard(directoryRotator_.currentDirectory().get());
    if (!dirGuard)
    {
        LOG(ERROR) << "Index directory is corrupted";
        return false;
    }
    indexManager_->optimizeIndex();
    return true;
}

void IndexWorker::doMining_()
{
    if (miningTaskService_)
    {
        std::string cronStr = miningTaskService_->getMiningBundleConfig()->mining_config_.dcmin_param.cron;
        if (cronStr.empty())
        {
            int docLimit = miningTaskService_->getMiningBundleConfig()->mining_config_.dcmin_param.docnum_limit;
            if (docLimit != 0 && (indexManager_->numDocs()) % docLimit == 0)
            {
                miningTaskService_->DoMiningCollection();
            }
        }
    }
}

bool IndexWorker::createDocument(const Value& documentValue)
{
    DirectoryGuard dirGuard(directoryRotator_.currentDirectory().get());
    if (!dirGuard)
    {
        LOG(ERROR) << "Index directory is corrupted";
        return false;
    }
    SCDDoc scddoc;
    value2SCDDoc(documentValue, scddoc);
    scd_writer_->Write(scddoc, INSERT_SCD);

    time_t timestamp = Utilities::createTimeStamp();

    Document document;
    IndexerDocument indexDocument, oldIndexDocument;
    docid_t oldId = 0;
    std::string source = "";
    IndexWorker::UpdateType updateType;
    if (!prepareDocument_(scddoc, document, indexDocument, oldIndexDocument, oldId, source, timestamp, updateType))
        return false;

    if(!indexManager_->isRealTime())
    	indexManager_->setIndexMode("realtime");

    bool ret = insertDoc_(document, indexDocument, timestamp, true);
    if (ret)
    {
        doMining_();
    }

    return ret;
}

bool IndexWorker::updateDocument(const Value& documentValue)
{
    DirectoryGuard dirGuard(directoryRotator_.currentDirectory().get());
    if (!dirGuard)
    {
        LOG(ERROR) << "Index directory is corrupted";
        return false;
    }
    SCDDoc scddoc;
    value2SCDDoc(documentValue, scddoc);

    time_t timestamp = Utilities::createTimeStamp();

    Document document;
    IndexerDocument indexDocument, oldIndexDocument;
    docid_t oldId = 0;
    IndexWorker::UpdateType updateType;
    std::string source = "";

    if (!prepareDocument_(scddoc, document, indexDocument, oldIndexDocument, oldId, source, timestamp, updateType, false))
    {
        return false;
    }

    if(!indexManager_->isRealTime())
    	indexManager_->setIndexMode("realtime");
    bool ret = updateDoc_(document, indexDocument, oldIndexDocument, timestamp, updateType, true);
    if (ret && (updateType != IndexWorker::RTYPE))
    {
        searchWorker_->clearSearchCache();
        doMining_();
    }

    if(updateType != IndexWorker::RTYPE)
    {
        documentManager_->getRTypePropertiesForDocument(document.getId(),document);
        document2SCDDoc(document,scddoc);
    }
    scd_writer_->Write(scddoc, UPDATE_SCD);

    return ret;
}

bool IndexWorker::updateDocumentInplace(const Value& request)
{
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
        if(!it->hasKey("property") || !it->hasKey("op") || !it->hasKey("opvalue"))
            return false;

        std::string propname = asString((*it)["property"]);
        std::string op = asString((*it)["op"]);
        std::string opvalue = asString((*it)["opvalue"]);
        PropertyValue oldvalue;
        // get old property first, if old property not exist, inplace update will fail.
        if(documentManager_->getPropertyValue(docid, propname, oldvalue))
        {
            tempPropertyConfig.propertyName_ = propname;
            IndexBundleSchema::const_iterator iter = bundleConfig_->indexSchema_.find(tempPropertyConfig);
            //DocumentSchema::const_iterator iter = bundleConfig_->documentSchema_.find(tempPropertyConfig);

            //Numeric property should not be range type
            bool isValidProperty = (iter != bundleConfig_->indexSchema_.end() && !iter->bRange_);
            int inplace_type = 0;
            // determine how to do the inplace operation by different property type.
            if(isValidProperty)
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
                if(oldvalue_str.empty()) oldvalue_str = "0";
                if(op == "add")
                {
                    if(inplace_type == 0 )
                    {
                        int64_t newv = boost::lexical_cast<int64_t>(oldvalue_str) + boost::lexical_cast<int64_t>(opvalue);
                        new_propvalue = boost::lexical_cast<std::string>(newv);
                    }
                    else if(inplace_type == 1)
                    {
                        new_propvalue = boost::lexical_cast<std::string>(boost::lexical_cast<float>(oldvalue_str) +
                                boost::lexical_cast<float>(opvalue));
                    }
                    else if(inplace_type == 2)
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
            catch(boost::bad_lexical_cast& e)
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

    // do the common update.
    return updateDocument(new_document);
}

bool IndexWorker::destroyDocument(const Value& documentValue)
{
    DirectoryGuard dirGuard(directoryRotator_.currentDirectory().get());
    if (!dirGuard)
    {
        LOG(ERROR) << "Index directory is corrupted";
        return false;
    }
    SCDDoc scddoc;
    value2SCDDoc(documentValue, scddoc);

    docid_t docid;
    uint128_t num_docid = Utilities::md5ToUint128(asString(documentValue["DOCID"]));

    if (!idManager_->getDocIdByDocName(num_docid, docid, false))
        return false;

    scd_writer_->Write(scddoc, DELETE_SCD);
    time_t timestamp = Utilities::createTimeStamp();
    bool ret = deleteDoc_(docid, timestamp);
    if (ret)
    {
        searchWorker_->clearSearchCache();
        doMining_();
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
    return indexManager_->numDocs();
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
        int op,
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
        timestamp = Utilities::createTimeStamp();

    if (op <= 2) // insert or update
    {
        bool isInsert = (op == 1);
        if (!insertOrUpdateSCD_(parser, isInsert, numdoc, timestamp))
            return false;
    }
    else //delete
    {
        if (!deleteSCD_(parser, timestamp))
            return false;
    }

    clearMasterCache_();

    saveSourceCount_(op);

    return true;
}

bool IndexWorker::insertOrUpdateSCD_(
        ScdParser& parser,
        bool isInsert,
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
        if (!prepareDocument_(*doc, document, indexDocument, oldIndexDocument, oldId, source, new_timestamp, updateType, isInsert))
            continue;

        if (!source.empty())
        {
            ++productSourceCount_[source];
        }

        if (isInsert || oldId == 0)
        {
            if (!insertDoc_(document, indexDocument, new_timestamp))
                continue;
        }
        else
        {
            if (!updateDoc_(document, indexDocument, oldIndexDocument, new_timestamp, updateType))
                continue;

            ++numUpdatedDocs_;
        }

        // interrupt when closing the process
        boost::this_thread::interruption_point();
    } // end of for loop for all documents
    flushUpdateBuffer_();
    searchWorker_->reset_all_property_cache();
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
        //LOG(WARNING) << "skip insert doc id " << docId << ", it is less than max DocId";
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
        }
        else
        {
            string property;
            iter->convertString(property, bundleConfig_->encoding_);
            //LOG(ERROR) << "Deleted document " << property << " does not exist, skip it";
        }
    }
    std::sort(docIdList.begin(), docIdList.end());

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

    searchWorker_->clearSearchCache();

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

    prepareIndexRTypeProperties_(document.getId(), indexDocument);
    if (hooker_)
    {
        ///Notice: the success of HookUpdate will not affect following update
        hooker_->HookUpdate(document, indexDocument, timestamp);
    }

    if (immediately)
        return doUpdateDoc_(document, indexDocument, oldIndexDocument, updateType);

    ///updateBuffer_ is used to change random IO in DocumentManager to sequential IO
    UpdateBufferDataType& updateData = updateBuffer_[document.getId()];
    updateData.get<0>() = updateType;
    updateData.get<1>().swap(document);
    updateData.get<2>().swap(indexDocument);
    updateData.get<3>().swap(oldIndexDocument);
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
            miningTaskService_->incDeletedDocBeforeMining();
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
            if(documentManager_->insertDocument(updateData.get<1>()))
            {
                LOG(INFO) << "doc id: " << it->first << " inserted in flush insert.";
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

            if(documentManager_->removeDocument(oldId))
            {
                miningTaskService_->incDeletedDocBeforeMining();
            }

            indexManager_->updateDocument(updateData.get<2>());

            if(!documentManager_->insertDocument(updateData.get<1>()))
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
        miningTaskService_->incDeletedDocBeforeMining();
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

void IndexWorker::saveSourceCount_(int op)
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
        if (op == 1)
        {
            productCount.setFlag("insert");
        }
        else if (op == 2)
        {
            productCount.setFlag("update");
        }
        else
        {
            productCount.setFlag("delete");
        }
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
        bool insert)
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
            if (!insert)
            {
                updateType = checkUpdateType_(scdDocId, doc, oldId, docId);
                if (updateType == RTYPE)
                    prepareIndexRTypeProperties_(oldId, oldIndexDocument);
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
                    if(iter->isRTypeString())
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

    for (Document::property_iterator it = oldDoc.propertyBegin(); it != oldDoc.propertyEnd(); ++it)
    {
        if (doc.hasProperty(it->first))
        {
            ///When new doc has same properties with old doc
            ///override content of old doc
            if (it->second == doc.property(it->first)) continue;
            if(boost::find_first(it->first,DocumentManager::PROPERTY_BLOCK_SUFFIX))
            {
                ///such properties are not UString type, so boost::get<> will throw exception
                it->second.swap(doc.property(it->first));
            }
            else
            {
                const izenelib::util::UString& newPropValue = doc.property(it->first).get<izenelib::util::UString>();
                if (newPropValue.empty()) continue;
                it->second = newPropValue;
            }
        }
        else if (generateIndexDoc)
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
    for(DocumentManager::RTypeStringPropTableMap::const_iterator rtype_it = rtype_string_proptable.begin();
        rtype_it != rtype_string_proptable.end(); ++rtype_it)
    {
        tempPropertyConfig.propertyName_ = rtype_it->first;
        IndexBundleSchema::iterator index_it = bundleConfig_->indexSchema_.find(tempPropertyConfig);
        if(index_it == bundleConfig_->indexSchema_.end())
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
        docid_t& docId)
{
    if (!idManager_->getDocIdByDocName(scdDocId, oldId, false))
    {
        idManager_->updateDocIdByDocName(scdDocId, docId);
        return INSERT;
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

        if( iter->isRTypeString() )
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

size_t IndexWorker::getTotalScdSize_()
{
    string scdPath = bundleConfig_->indexSCDPath();

    ScdParser parser(bundleConfig_->encoding_);

    size_t sizeInBytes = 0;
    // search the directory for files
    static const bfs::directory_iterator kItrEnd;
    for (bfs::directory_iterator itr(scdPath); itr != kItrEnd; ++itr)
    {
        if (bfs::is_regular_file(itr->status()))
        {
            std::string fileName = itr->path().filename().string();
            if (parser.checkSCDFormat(fileName))
            {
                parser.load(scdPath + fileName);
                sizeInBytes += parser.getFileSize();
            }
        }
    }
    return sizeInBytes/(1024*1024);
}

bool IndexWorker::requireBackup_(size_t currTotalScdSizeInMB)
{
    static size_t threshold = 200;//200M
    totalSCDSizeSinceLastBackup_ += currTotalScdSizeInMB;
    const boost::shared_ptr<Directory>& current
    = directoryRotator_.currentDirectory();
    const boost::shared_ptr<Directory>& next
    = directoryRotator_.nextDirectory();
    if (next
            && current->name() != next->name())
        //&& ! (next->valid() && next->parentName() == current->name()))
    {
        ///TODO policy required here
        if (totalSCDSizeSinceLastBackup_ > threshold)
            return true;
    }
    return false;
}

bool IndexWorker::backup_()
{
    const boost::shared_ptr<Directory>& current
    = directoryRotator_.currentDirectory();
    const boost::shared_ptr<Directory>& next
    = directoryRotator_.nextDirectory();

    // valid pointer
    // && not the same directory
    // && have not copied successfully yet
    if (next
            && current->name() != next->name())
        //&& ! (next->valid() && next->parentName() == current->name()))
    {
        try
        {
            LOG(INFO) << "Copy index dir from " << current->name()
                      << " to " << next->name();
            next->copyFrom(*current);
            return true;
        }
        catch (bfs::filesystem_error& e)
        {
            LOG(ERROR) << "Failed to copy index directory " << e.what();
        }

        // try copying but failed
        return false;
    }

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
        scddoc[insertto].second.assign(
            asString(it->second),
            izenelib::util::UString::UTF_8
            );
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
