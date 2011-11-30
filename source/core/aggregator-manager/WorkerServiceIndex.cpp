#include "WorkerService.h"
#include "WorkerHelper.h"

#include <index-manager/IndexManager.h>
#include <index-manager/IndexHooker.h>
#include <index-manager/IndexModeSelector.h>
#include <search-manager/SearchManager.h>
#include <mining-manager/MiningManager.h>
#include <document-manager/DocumentManager.h>
#include <la-manager/LAManager.h>
#include <log-manager/ProductCount.h>
#include <common/JobScheduler.h>
#include <common/Utilities.h>
//#include <license-manager/LicenseManager.h>
//#include <process/common/CollectionMeta.h>
//#include <process/common/XmlConfigParser.h>

#include <bundles/index/IndexBundleConfiguration.h>

#include <util/profiler/ProfilerGroup.h>

#include <la/util/UStringUtil.h>

#include <glog/logging.h>

#include <boost/filesystem.hpp>

namespace bfs = boost::filesystem;

using izenelib::util::UString;

namespace sf1r
{

namespace
{
/** the directory for scd file backup */
const char* SCD_BACKUP_DIR = "backup";
}

bool WorkerService::index(const unsigned int& numdoc, bool& ret)
{
    task_type task = boost::bind(&WorkerService::buildCollection, this, numdoc);
    JobScheduler::get()->addTask(task);

    ret = true;
    return ret;
}

bool WorkerService::buildCollection(unsigned int numdoc)
{
    string scdPath = bundleConfig_->collPath_.getScdPath() + "index/";

    if(!backup_() )
        return false;

    DirectoryGuard dirGuard(directoryRotator_.currentDirectory().get());
    if (!dirGuard)
    {
        LOG(ERROR) << "Index directory is corrupted";
        return false;
    }

    sf1r::Status::Guard statusGuard(indexStatus_);
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
        if (bfs::is_directory(scdPath) == false)
        {
            LOG(ERROR) << "SCD Path does not exist. Path " << scdPath;
            return false;
        }
    }
    catch(bfs::filesystem_error& e)
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
            std::string fileName = itr->path().filename();
            if (parser.checkSCDFormat(fileName) )
            {
                scdList.push_back(itr->path().string() );
                parser.load(scdPath+fileName);
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

    LOG(INFO) << "SCD Files in Path processed in given order. Path " << scdPath;
    vector<string>::iterator scd_it;
    for ( scd_it = scdList.begin(); scd_it != scdList.end(); ++scd_it)
        LOG(INFO) << "SCD File " << bfs::path(*scd_it).stem();

    try
    {
        // loops the list of SCD files that belongs to this collection
        long proccessedFileSize = 0;
        for ( scd_it = scdList.begin(); scd_it != scdList.end(); scd_it++ )
        {
            size_t pos = scd_it ->rfind("/")+1;
            string filename = scd_it ->substr(pos);
            indexProgress_.currentFileName = filename;
            indexProgress_.currentFilePos_ = 0;

            LOG(INFO) << "Processing SCD file. " << bfs::path(*scd_it).stem();

            switch ( parser.checkSCDType(*scd_it) )
            {
            case INSERT_SCD:
            {
                if (doBuildCollection_( *scd_it, 1, numdoc ) == false)
                {
                    //continue;
                }
                LOG(INFO) << "Indexing Finished";

            }
            break;
            case DELETE_SCD:
            {
                if (documentManager_->getMaxDocId()> 0)
                {
                    doBuildCollection_( *scd_it, 3, 0 );
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
                doBuildCollection_( *scd_it, 2, 0 );
                LOG(INFO) << "Update Finished";
            }
            break;
            default:
                break;
            }
            parser.load( *scd_it );
            proccessedFileSize += parser.getFileSize();
            indexProgress_.totalFilePos_ = proccessedFileSize;
            indexProgress_.getIndexingStatus(indexStatus_);
            ++indexProgress_.currentFileIdx;

        } // end of loop for scd files of a collection

        documentManager_->flush();
        idManager_->flush();
        index_mode_selector.TryCommit();
        //indexManager_->optimizeIndex();
        if( bundleConfig_->isTrieWildcard()) {
#ifdef __x86_64
            idManager_->startWildcardProcess();
            idManager_->joinWildcardProcess();
#endif
        }

        if(hooker_)
        {
            if(!hooker_->Finish())
            {
                std::cout<<"[IndexTaskService] Hooker Finish failed."<<std::endl;
                return false;
            }
            std::cout<<"[IndexTaskService] Hooker Finished."<<std::endl;
        }

        if( miningManager_ )
        {
          indexManager_->pauseMerge();
          miningManager_->DoMiningCollection();
          indexManager_->resumeMerge();
        }
    }
    catch (std::exception& e)
    {
        LOG(WARNING) << "exception in indexing or mining: " << e.what();
        indexProgress_.getIndexingStatus(indexStatus_);
        indexProgress_.reset();
        return false;
    }
    indexManager_->getIndexReader();

    bfs::path bkDir = bfs::path(scdPath) / SCD_BACKUP_DIR;
    bfs::create_directory(bkDir);
    LOG(INFO) << "moving " << scdList.size() << " SCD files to directory " << bkDir;
    for (scd_it = scdList.begin(); scd_it != scdList.end(); ++scd_it)
    {
        try
        {
            bfs::rename(*scd_it, bkDir / bfs::path(*scd_it).filename());
        }
        catch(bfs::filesystem_error& e)
        {
            LOG(WARNING) << "exception in rename file " << *scd_it << ": " << e.what();
        }
    }

    indexProgress_.getIndexingStatus(indexStatus_);
    LOG(INFO) << "Indexing Finished! Documents Indexed: " << documentManager_->getMaxDocId()
              << " Deleted: " << numDeletedDocs_
              << " Updated: " << numUpdatedDocs_;

    //both variables are refreshed
    numDeletedDocs_ = 0;
    numUpdatedDocs_ = 0;

    indexProgress_.reset();
    STOP_PROFILER(buildIndex);
    REPORT_PROFILE_TO_FILE( "PerformanceIndexResult.SIAProcess" )
    LOG(INFO) << "End BuildCollection: ";
    LOG(INFO) << "time elapsed:" << timer.elapsed() <<"seconds";

    return true;
}

bool WorkerService::getPropertyValue_(const PropertyValue& value, std::string& valueStr)
{
    try
    {
        izenelib::util::UString sourceFieldValue = get<izenelib::util::UString>( value );
        sourceFieldValue.convertString(valueStr, izenelib::util::UString::UTF_8);
        return true;
    }
    catch(boost::bad_get& e)
    {
        LOG(WARNING) << "exception in get property value: " << e.what();
        return false;
    }
}

bool WorkerService::backup_()
{
    const boost::shared_ptr<Directory>& current
        = directoryRotator_.currentDirectory();
    const boost::shared_ptr<Directory>& next
        = directoryRotator_.nextDirectory();

    // valid pointer
    // && not the same directory
    // && have not copied successfully yet
    if (next
        && current->name() != next->name()
        && ! (next->valid() && next->parentName() == current->name()))
    {
        try
        {
            LOG(INFO) << "Copy index dir from " << current->name()
                      << " to " << next->name();
            next->copyFrom(*current);
            return true;
        }
        catch(bfs::filesystem_error& e)
        {
            LOG(ERROR) << "Failed to copy index directory " << e.what();
        }

        // try copying but failed
        return false;
    }

    // not copy, always returns true
    return true;
}

bool WorkerService::doBuildCollection_(
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
    // Timestamp: YYYYMMDDThhmmss,fff
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

    saveSourceCount_(op);

    return true;
}

bool WorkerService::insertOrUpdateSCD_(
        ScdParser& parser,
        bool isInsert,
        uint32_t numdoc,
        time_t timestamp)
{
    CREATE_SCOPED_PROFILER (insertOrUpdateSCD, "IndexTaskService", "IndexTaskService::insertOrUpdateSCD_");

    uint32_t n = 0;
    long lastOffset = 0;
    for (ScdParser::iterator doc_iter = parser.begin(propertyList_);
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
        Document document;
        IndexerDocument indexDocument;
        docid_t oldId = 0;
        bool rType = false;
        std::map<std::string, pair<PropertyDataType, izenelib::util::UString> > rTypeFieldValue;
        sf1r::docid_t id = 0;
        std::string source = "";
        time_t new_timestamp = timestamp;

        if (!prepareDocument_(*doc, document, oldId, rType, rTypeFieldValue, source, new_timestamp, isInsert))
            continue;

        prepareIndexDocument_(oldId, document, indexDocument);

        if (!source.empty())
        {
            ++productSourceCount_[source];
        }

        if (rType)
        {
            id = document.getId();
        }

        if (isInsert || oldId == 0)
        {
            if (!insertDoc_(document, indexDocument, new_timestamp))
                continue;
        }
        else
        {
            if (!updateDoc_(document, indexDocument, new_timestamp, rType))
                continue;

            ++numUpdatedDocs_;
        }
        searchManager_->reset_cache(rType, id, rTypeFieldValue);

        // interrupt when closing the process
        boost::this_thread::interruption_point();
    } // end of for loop for all documents

    return true;
}

bool WorkerService::createUpdateDocId_(
        const izenelib::util::UString& scdDocId,
        bool rType,
        docid_t& oldId,
        docid_t& newId)
{
    bool result = false;

    if (rType)
    {
        if ((result = idManager_->getDocIdByDocName(scdDocId, oldId, false)))
        {
            newId = oldId;
        }
    }
    else
    {
        result = idManager_->updateDocIdByDocName(scdDocId, oldId, newId);
    }

    return result;
}

bool WorkerService::createInsertDocId_(
        const izenelib::util::UString& scdDocId,
        docid_t& newId)
{
    docid_t docId = 0;

    // already converted
    if (idManager_->getDocIdByDocName(scdDocId, docId))
    {
        if (documentManager_->isDeleted(docId))
        {
            docid_t oldId = 0;
            if (! idManager_->updateDocIdByDocName(scdDocId, oldId, docId))
            {
                //LOG(WARNING) << "doc id " << docId << " should have been converted";
                return false;
            }
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

bool WorkerService::deleteSCD_(ScdParser& parser, time_t timestamp)
{
    std::vector<izenelib::util::UString> rawDocIDList;
    if (!parser.getDocIdList(rawDocIDList))
    {
        LOG(WARNING) << "SCD File not valid.";
        return false;
    }

    //get the docIds for deleting
    std::vector<sf1r::docid_t> docIdList;
    docIdList.reserve(rawDocIDList.size());
    indexProgress_.currentFileSize_ =rawDocIDList.size();
    indexProgress_.currentFilePos_ = 0;
    for (std::vector<izenelib::util::UString>::iterator iter = rawDocIDList.begin();
        iter != rawDocIDList.end(); ++iter)
    {
        sf1r::docid_t docId;
        if (idManager_->getDocIdByDocName(*iter, docId, false))
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
    for (std::vector<sf1r::docid_t>::iterator iter = docIdList.begin(); iter
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

    std::map<std::string, pair<PropertyDataType, izenelib::util::UString> > rTypeFieldValue;
    searchManager_->reset_cache(false, 0, rTypeFieldValue);

    // interrupt when closing the process
    boost::this_thread::interruption_point();

    return true;
}

bool WorkerService::insertDoc_(Document& document, IndexerDocument& indexDocument, time_t timestamp)
{
    CREATE_PROFILER(proDocumentIndexing, "IndexTaskService", "IndexTaskService : InsertDocument")
    CREATE_PROFILER(proIndexing, "IndexTaskService", "IndexTaskService : indexing")

    if (hooker_)
    {
        if (!hooker_->HookInsert(document, indexDocument, timestamp)) return false;
    }
    START_PROFILER(proDocumentIndexing);
    if (documentManager_->insertDocument(document))
    {
        STOP_PROFILER(proDocumentIndexing);

        START_PROFILER(proIndexing);
        indexManager_->insertDocument(indexDocument);
        STOP_PROFILER(proIndexing);
        indexStatus_.numDocs_ = indexManager_->getIndexReader()->numDocs();
        return true;
    }
    else return false;
}

bool WorkerService::updateDoc_(
        Document& document,
        IndexerDocument& indexDocument,
        time_t timestamp,
        bool rType)
{
    CREATE_SCOPED_PROFILER (proDocumentUpdating, "IndexTaskService", "IndexTaskService::UpdateDocument");

    if (hooker_)
    {
        if (!hooker_->HookUpdate(document, indexDocument, timestamp, rType)) return false;
    }
    if (rType)
    {
        // Store the old property value.
        IndexerDocument oldIndexDocument;
        if (!preparePartialDocument_(document, oldIndexDocument))
            return false;

        // Update document data in the SDB repository.
        if (!documentManager_->updatePartialDocument(document))
        {
            LOG(ERROR) << "Document Update Failed in SDB. " << document.property("DOCID");
            return false;
        }

        indexManager_->updateRtypeDocument(oldIndexDocument, indexDocument);
    }
    else
    {
        uint32_t oldId = indexDocument.getId();
        if (!documentManager_->removeDocument(oldId))
        {
            //LOG(WARNING) << "document " << oldId << " is already deleted";
        }
        if (!documentManager_->insertDocument(document))
        {
            LOG(ERROR) << "Document Insert Failed in SDB. " << document.property("DOCID");
            return false;
        }

        indexManager_->updateDocument(indexDocument);
    }

    return true;
}

bool WorkerService::deleteDoc_(docid_t docid, time_t timestamp)
{
    CREATE_SCOPED_PROFILER (proDocumentDeleting, "IndexTaskService", "IndexTaskService::DeleteDocument");

    if (hooker_)
    {
        if (!hooker_->HookDelete(docid, timestamp)) return false;
    }
    if (documentManager_->removeDocument(docid))
    {
        indexManager_->removeDocument(collectionId_, docid);
        ++numDeletedDocs_;
        indexStatus_.numDocs_ = indexManager_->getIndexReader()->numDocs();
        return true;
    }
    else return false;
}

void WorkerService::saveSourceCount_(int op)
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

bool WorkerService::prepareDocument_(
        SCDDoc& doc,
        Document& document,
        docid_t& oldId,
        bool& rType,
        std::map<std::string, pair<PropertyDataType, izenelib::util::UString> >& rTypeFieldValue,
        std::string& source,
        time_t& timestamp,
        bool insert)
{
    CREATE_SCOPED_PROFILER (preparedocument, "IndexTaskService", "IndexTaskService::prepareDocument_");

    sf1r::docid_t docId = 0;
    string fieldStr;
    vector<CharacterOffset> sentenceOffsetList;
    AnalysisInfo analysisInfo;
    if (doc.empty()) return false;
    // the iterator is not const because the p-second value may change
    // due to the maxlen setting

    vector<pair<izenelib::util::UString, izenelib::util::UString> >::iterator p;
    bool dateExistInSCD = false;

    for (p = doc.begin(); p != doc.end(); p++)
    {
        bool extraProperty = false;
        std::set<PropertyConfig, PropertyComp>::iterator iter;
        p->first.convertString(fieldStr, bundleConfig_->encoding_);

        PropertyConfig temp;
        temp.propertyName_ = fieldStr;
        iter = bundleConfig_->schema_.find(temp);

        IndexerPropertyConfig indexerPropertyConfig;
        if (iter == bundleConfig_->schema_.end())
            extraProperty = true;

        izenelib::util::UString propertyNameL = p->first;
        propertyNameL.toLowerString();
        const izenelib::util::UString & propertyValueU = p->second; // preventing copy

        izenelib::util::UString::EncodingType encoding = bundleConfig_->encoding_;
        std::string fieldValue("");
        propertyValueU.convertString(fieldValue, encoding);

        if (!bundleConfig_->productSourceField_.empty()
              && fieldStr == bundleConfig_->productSourceField_)
        {
            source = fieldValue;
        }

        if (propertyNameL == izenelib::util::UString("docid", encoding)
                && !extraProperty)
        {
            // update
            if (!insert)
            {
                rType = checkRtype_(doc, rTypeFieldValue);
                if (rType && rTypeFieldValue.empty())
                {
                    //LOG(WARNING) << "skip updating SCD DOC " << fieldValue << ", as none of its property values is changed";
                    return false;
                }

                if (! createUpdateDocId_(propertyValueU, rType, oldId, docId))
                    insert = true;
            }

            if (insert && !createInsertDocId_(propertyValueU, docId))
            {
                //LOG(WARNING) << "failed to create id for SCD DOC " << fieldValue;
                return false;
            }

            document.setId(docId);
            document.property(fieldStr) = propertyValueU;
        }
        else if (propertyNameL == izenelib::util::UString("date", encoding))
        {
            /// format <DATE>20091009163011
            dateExistInSCD = true;
            izenelib::util::UString dateStr;
            sf1r::Utilities::convertDate(propertyValueU, encoding, dateStr);
            document.property(dateProperty_.getName()) = dateStr;
        }
        else if (!extraProperty)
        {
            if (iter->getType() == STRING_PROPERTY_TYPE)
            {
                document.property(fieldStr) = propertyValueU;
                analysisInfo.clear();
                analysisInfo = iter->getAnalysisInfo();
                if (!analysisInfo.analyzerId_.empty())
                {
                    unsigned int numOfSummary = 0;
                    if (iter->getIsSummary())
                    {
                        numOfSummary = iter->getSummaryNum();
                        if (numOfSummary <= 0)
                            numOfSummary = 1; //atleast one sentence required for summary

                        if (!makeSentenceBlocks_(propertyValueU, iter->getDisplayLength(),
                                                numOfSummary, sentenceOffsetList))
                        {
                            LOG(ERROR) << "Make Sentence Blocks Failes ";
                        }

                        document.property(fieldStr + ".blocks") = sentenceOffsetList;
                    }
                }
            }
            else if (iter->getType() == INT_PROPERTY_TYPE
                    || iter->getType() == FLOAT_PROPERTY_TYPE
                    || iter->getType() == NOMINAL_PROPERTY_TYPE)
            {
                document.property(fieldStr) = propertyValueU;
            }
            else
            {
            }
        }
    }

    if (dateExistInSCD) timestamp = -1;
    else
    {
        std::string dateStr = boost::posix_time::to_iso_string(boost::posix_time::from_time_t(timestamp / 1000000 - timezone));
        document.property(dateProperty_.getName()) = izenelib::util::UString(dateStr.erase(8, 1), izenelib::util::UString::UTF_8);
    }

    if (!insert && !rType)
    {
        if (!completePartialDocument_(oldId, document))
             return false;
    }
    return true;
}


bool WorkerService::prepareIndexDocument_(
        docid_t oldId,
        const Document& document,
        IndexerDocument& indexDocument)
{
    CREATE_SCOPED_PROFILER (preparedocument, "IndexTaskService", "IndexTaskService::prepareIndexDocument_");

    sf1r::docid_t docId = document.getId();//new id;
    izenelib::util::UString::EncodingType encoding = bundleConfig_->encoding_;
    string fieldStr;
    AnalysisInfo analysisInfo;
    typedef Document::property_const_iterator document_iterator;
    document_iterator p;
    // the iterator is not const because the p-second value may change
    // due to the maxlen setting
    for (p = document.propertyBegin(); p != document.propertyEnd(); ++p)
    {
        std::set<PropertyConfig, PropertyComp>::iterator iter;
        fieldStr = p->first;

        PropertyConfig temp;
        temp.propertyName_ = fieldStr;
        iter = bundleConfig_->schema_.find(temp);

        IndexerPropertyConfig indexerPropertyConfig;

        izenelib::util::UString propertyNameL = izenelib::util::UString(fieldStr, encoding);
        propertyNameL.toLowerString();
        const izenelib::util::UString & propertyValueU = *(get<izenelib::util::UString>(&(p->second)));
        indexerPropertyConfig.setPropertyId(iter->getPropertyId());
        indexerPropertyConfig.setName(iter->getName());
        indexerPropertyConfig.setIsIndex(iter->isIndex());
        indexerPropertyConfig.setIsAnalyzed(iter->isAnalyzed());
        indexerPropertyConfig.setIsFilter(iter->getIsFilter());
        indexerPropertyConfig.setIsMultiValue(iter->getIsMultiValue());
        indexerPropertyConfig.setIsStoreDocLen(iter->getIsStoreDocLen());


        if (propertyNameL == izenelib::util::UString("docid", encoding))
        {
            indexDocument.setId(oldId);
            indexDocument.setDocId(docId, collectionId_);
        }
        else if (propertyNameL == izenelib::util::UString("date", encoding))
        {
            /// format <DATE>20091009163011
            izenelib::util::UString dateStr;
            int64_t time = sf1r::Utilities::convertDate(propertyValueU, encoding, dateStr);
            indexerPropertyConfig.setPropertyId(dateProperty_.getPropertyId());
            indexerPropertyConfig.setName(dateProperty_.getName());
            indexerPropertyConfig.setIsIndex(true);
            indexerPropertyConfig.setIsFilter(true);
            indexerPropertyConfig.setIsAnalyzed(false);
            indexerPropertyConfig.setIsMultiValue(false);
            indexDocument.insertProperty(indexerPropertyConfig, time);
        }
        else
        {
            if (iter->getType() == STRING_PROPERTY_TYPE)
            {
                if (!propertyValueU.empty())
                {
                    ///process for properties that requires forward index to be created
                    if (iter->isIndex())
                    {
                        analysisInfo.clear();
                        analysisInfo = iter->getAnalysisInfo();
                        if (analysisInfo.analyzerId_.empty())
                        {
                            if (iter->getIsFilter() && iter->getIsMultiValue())
                            {
                                MultiValuePropertyType props;
                                split_string(propertyValueU,props, encoding,',');
                                indexDocument.insertProperty(indexerPropertyConfig, props);
                            }
                            else
                                indexDocument.insertProperty(indexerPropertyConfig,
                                                          propertyValueU);
                        }
                        else
                        {
                            laInputs_[iter->getPropertyId()]->setDocId(docId);
                            if (!makeForwardIndex_(propertyValueU, fieldStr, iter->getPropertyId(), analysisInfo))
                            {
                                LOG(ERROR) << "Forward Indexing Failed Error Line : " << __LINE__;
                                return false;
                            }
                            if (iter->getIsFilter())
                            {
                                if (iter->getIsMultiValue())
                                {
                                    MultiValuePropertyType props;
                                    split_string(propertyValueU,props, encoding,',');

                                    MultiValueIndexPropertyType
                                    indexData = std::make_pair(laInputs_[iter->getPropertyId()],props);
                                    indexDocument.insertProperty(indexerPropertyConfig, indexData);

                                }
                                else
                                {
                                    IndexPropertyType
                                    indexData =
                                        std::make_pair(
                                            laInputs_[iter->getPropertyId()],
                                            const_cast<izenelib::util::UString &>(propertyValueU));
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
                                    laInputs_[vecIter->getPropertyId()]->setDocId(docId);
                                    if (!makeForwardIndex_(
                                                propertyValueU,
                                                fieldStr,
                                                vecIter->getPropertyId(),
                                                aliasAnalysisInfo))
                                    {
                                        LOG(ERROR) << "Forward Indexing Failed Error Line : " << __LINE__;
                                        return false;
                                    }
                                    IndexerPropertyConfig
                                    aliasIndexerPropertyConfig(
                                        vecIter->getPropertyId(),
                                        vecIter->getName(),
                                        vecIter->isIndex(),
                                        vecIter->isAnalyzed());
                                    aliasIndexerPropertyConfig.setIsFilter(vecIter->getIsFilter());
                                    aliasIndexerPropertyConfig.setIsMultiValue(vecIter->getIsMultiValue());
                                    aliasIndexerPropertyConfig.setIsStoreDocLen(vecIter->getIsStoreDocLen());
                                    indexDocument.insertProperty(
                                        aliasIndexerPropertyConfig,laInputs_[vecIter->getPropertyId()]);
                                } // end - for
                            } // end - if (mapIter != end())

                        }
                    }
                    // insert property name and value for other properties that is not DOCID and neither required to be indexed
                    else
                    {
                        //other extra properties that need not be in index manager
                        indexDocument.insertProperty(indexerPropertyConfig,
                                                      propertyValueU);
                    }
                }
            }
            else if (iter->getType() == INT_PROPERTY_TYPE)
            {
                if (iter->isIndex())
                {
                    if (iter->getIsMultiValue())
                    {
                        MultiValuePropertyType props;
                        split_int(propertyValueU,props, encoding,',');
                        indexDocument.insertProperty(indexerPropertyConfig, props);
                    }
                    else
                    {
                        std::string str("");
                        propertyValueU.convertString(str, encoding);
                        int64_t value = 0;
                        try
                        {
                            value = boost::lexical_cast< int64_t >(str);
                            indexDocument.insertProperty(indexerPropertyConfig, value);
                        }
                        catch (const boost::bad_lexical_cast &)
                        {
                            MultiValuePropertyType multiProps;
                            if (checkSeparatorType_(propertyValueU, encoding, '-'))
                            {
                                split_int(propertyValueU, multiProps, encoding,'-');
                            }
                            else if (checkSeparatorType_(propertyValueU, encoding, '~'))
                            {
                                split_int(propertyValueU, multiProps, encoding,'~');
                            }
                            else if (checkSeparatorType_(propertyValueU, encoding, ','))
                            {
                                split_int(propertyValueU, multiProps, encoding,',');
                            }
                            else
                            {
                                LOG(ERROR) << "Wrong format of number value. DocId " << docId << " Value" << str;
                            }
                            indexerPropertyConfig.setIsMultiValue(true);
                            indexDocument.insertProperty(indexerPropertyConfig, multiProps);
                            //sflog->error(SFL_IDX,10140, docId);
                        }
                    }
                }
            }
            else if (iter->getType() == FLOAT_PROPERTY_TYPE)
            {
                if (iter->isIndex())
                {
                    if (iter->getIsMultiValue())
                    {
                        MultiValuePropertyType props;
                        split_float(propertyValueU,props, encoding,',');
                        indexDocument.insertProperty(indexerPropertyConfig, props);
                    }
                    else
                    {
                        std::string str("");
                        propertyValueU.convertString(str, encoding);
                        float value = 0;
                        try
                        {
                            value = boost::lexical_cast< float >(str);
                            indexDocument.insertProperty(indexerPropertyConfig, value);
                        }
                        catch (const boost::bad_lexical_cast &)
                        {
                            MultiValuePropertyType multiProps;
                            if (checkSeparatorType_(propertyValueU, encoding, '-'))
                            {
                                split_float(propertyValueU, multiProps, encoding,'-');
                            }
                            else if (checkSeparatorType_(propertyValueU, encoding, '~'))
                            {
                                split_float(propertyValueU, multiProps, encoding,'~');
                            }
                            else if (checkSeparatorType_(propertyValueU, encoding, ','))
                            {
                                split_float(propertyValueU, multiProps, encoding,',');
                            }
                            indexerPropertyConfig.setIsMultiValue(true);
                            indexDocument.insertProperty(indexerPropertyConfig, multiProps);
                        }
                    }
                }
            }
            else
            {
            }
        }
    }

    return true;
}

bool WorkerService::preparePartialDocument_(
        Document& document,
        IndexerDocument& oldIndexDocument)
{
    // Store the old property value.
    sf1r::docid_t docId = document.getId();
    Document oldDoc;

    if ( !documentManager_->getDocument(docId, oldDoc) )
    {
        return false;
    }

    typedef Document::property_const_iterator iterator;
    for (iterator it = document.propertyBegin(), itEnd = document.propertyEnd(); it
                 != itEnd; ++it) {
        if (it->first != "DOCID" && it->first != "DATE" ) {
            std::set<PropertyConfig, PropertyComp>::iterator iter;
            PropertyConfig temp;
            temp.propertyName_ = it->first;
            iter = bundleConfig_->schema_.find(temp);

            //set indexerPropertyConfig
            IndexerPropertyConfig indexerPropertyConfig;
            if (iter == bundleConfig_->schema_.end())
            {
                continue;
            }

            if(iter->isIndex() && iter->getIsFilter())
            {
                indexerPropertyConfig.setPropertyId(iter->getPropertyId());
                indexerPropertyConfig.setName(iter->getName());
                indexerPropertyConfig.setIsIndex(iter->isIndex());
                indexerPropertyConfig.setIsAnalyzed(iter->isAnalyzed());
                indexerPropertyConfig.setIsFilter(iter->getIsFilter());
                indexerPropertyConfig.setIsMultiValue(iter->getIsMultiValue());
                indexerPropertyConfig.setIsStoreDocLen(iter->getIsStoreDocLen());

                PropertyValue propertyValue = oldDoc.property(it->first);
                const izenelib::util::UString* stringValue = get<izenelib::util::UString>(&propertyValue);

                izenelib::util::UString::EncodingType encoding = bundleConfig_->encoding_;
                std::string str("");
                stringValue->convertString(str, encoding);
                if ( iter->getType() == INT_PROPERTY_TYPE )
                {
                    if(iter->getIsMultiValue())
                    {
                        MultiValuePropertyType props;
                        split_int(*stringValue, props, encoding, ',');
                        oldIndexDocument.insertProperty(indexerPropertyConfig, props);
                    }
                    else
                    {
                        int64_t value = 0;
                        try
                        {
                            value = boost::lexical_cast< int64_t >( str );
                            oldIndexDocument.insertProperty(indexerPropertyConfig, value);
                        }
                        catch( const boost::bad_lexical_cast & )
                        {
                            MultiValuePropertyType props;
                            if( checkSeparatorType_(*stringValue, encoding, '-') )
                            {
                                split_int(*stringValue, props, encoding,'-');
                            }
                            else if( checkSeparatorType_(*stringValue, encoding, '~') )
                            {
                                split_int(*stringValue, props, encoding,'~');
                            }
                            else if( checkSeparatorType_(*stringValue, encoding, ',') )
                            {
                                split_int(*stringValue, props, encoding,',');
                            }
                            indexerPropertyConfig.setIsMultiValue(true);
                            oldIndexDocument.insertProperty(indexerPropertyConfig, props);
                         }
                    }
                }
                else if ( iter->getType() == FLOAT_PROPERTY_TYPE )
                {
                    if(iter->getIsMultiValue())
                    {
                        MultiValuePropertyType props;
                        split_float(*stringValue, props, encoding,',');
                        oldIndexDocument.insertProperty(indexerPropertyConfig, props);
                    }
                    else
                    {
                        float value = 0.0;
                        try
                        {
                            value = boost::lexical_cast< float >( str );
                            oldIndexDocument.insertProperty(indexerPropertyConfig, value);
                        }
                        catch( const boost::bad_lexical_cast & )
                        {
                            MultiValuePropertyType props;
                            if( checkSeparatorType_(*stringValue, encoding, '-') )
                            {
                                split_float(*stringValue, props, encoding,'-');
                            }
                            else if( checkSeparatorType_(*stringValue, encoding, '~') )
                            {
                                split_float(*stringValue, props, encoding,'~');
                            }
                            else if( checkSeparatorType_(*stringValue, encoding, ',') )
                            {
                                split_float(*stringValue, props, encoding,',');
                            }
                            indexerPropertyConfig.setIsMultiValue(true);
                            oldIndexDocument.insertProperty(indexerPropertyConfig, props);
                         }
                    }
                }
            }
        }
    }
    return true;
}

bool WorkerService::completePartialDocument_(docid_t oldId, Document& doc)
{
    docid_t newId = doc.getId();
    Document oldDoc;
    if (!documentManager_->getDocument(oldId, oldDoc))
    {
        return false;
    }

    oldDoc.copyPropertiesFromDocument(doc);

    doc.swap(oldDoc);
    doc.setId(newId);
    return true;
}

bool WorkerService::checkSeparatorType_(const izenelib::util::UString& propertyValueStr, izenelib::util::UString::EncodingType encoding, char separator)
{
    izenelib::util::UString tmpStr(propertyValueStr);
    izenelib::util::UString sep(" ",encoding);
    sep[0] = separator;
    size_t n = 0;
    n = tmpStr.find(sep,0);
    if (n != izenelib::util::UString::npos)
        return true;
    return false;
}

bool WorkerService::checkRtype_(
        SCDDoc& doc,
        std::map<std::string, pair<PropertyDataType, izenelib::util::UString> >& rTypeFieldValue)
{
    //R-type check
    bool rType = false;
    PropertyDataType dataType;
    sf1r::docid_t docId;
    izenelib::util::UString newPropertyValue, oldPropertyValue;
    vector<pair<izenelib::util::UString, izenelib::util::UString> >::iterator p;
    for (p = doc.begin(); p != doc.end(); p++)
    {
        izenelib::util::UString propertyNameL = p->first;
        propertyNameL.toLowerString();
        const izenelib::util::UString & propertyValueU = p->second;
        std::set<PropertyConfig, PropertyComp>::iterator iter;
        string fieldName;
        p->first.convertString(fieldName, bundleConfig_->encoding_ );

        PropertyConfig tempPropertyConfig;
        tempPropertyConfig.propertyName_ = fieldName;
        iter = bundleConfig_->schema_.find(tempPropertyConfig);

        if ( iter != bundleConfig_->schema_.end() )
        {
            if ( propertyNameL == izenelib::util::UString("docid", bundleConfig_->encoding_) )
            {
                if (!idManager_->getDocIdByDocName(propertyValueU, docId, false))
                    break;
            }
            else
            {
                newPropertyValue = propertyValueU;
                if( propertyNameL == izenelib::util::UString("date", bundleConfig_->encoding_) )
                {
                    izenelib::util::UString dateStr;
                    sf1r::Utilities::convertDate(propertyValueU, bundleConfig_->encoding_, dateStr);
                    newPropertyValue = dateStr;
                }

                string newValueStr(""), oldValueStr("");
                newPropertyValue.convertString(newValueStr, izenelib::util::UString::UTF_8);

                PropertyValue value;
                if (documentManager_->getPropertyValue(docId, iter->getName(), value))
                {
                    if(getPropertyValue_(value, oldValueStr))
                    {
                        if (newValueStr == oldValueStr)
                            continue;
                    }
                    else
                    {
                        return false;
                    }
                }

                if ( iter->isIndex() && iter->getIsFilter() && !iter->isAnalyzed())
                {
                    dataType = iter->getType();
                    if ( dataType != INT_PROPERTY_TYPE && dataType != UNSIGNED_INT_PROPERTY_TYPE
                        && dataType != FLOAT_PROPERTY_TYPE && dataType != DOUBLE_PROPERTY_TYPE )
                    {
                        break;
                    }
                    pair<PropertyDataType, izenelib::util::UString> fieldValue;
                    fieldValue.first = dataType;
                    fieldValue.second = newPropertyValue;
                    rTypeFieldValue[iter->getName()] = fieldValue;
                }
                else
                {
                    break;
                }
            }
        }
        else
        {
            break;
        }
    }
    if ( p == doc.end() )
    {
        rType = true;
    }
    else
    {
        rType = false;
        rTypeFieldValue.clear();
    }
    return rType;
}

/// @desc Make a forward index of a given text.
/// You can specify an Language Analysis option through AnalysisInfo parameter.
/// You have to get a proper AnalysisInfo value from the configuration. (Currently not implemented.)
bool WorkerService::makeForwardIndex_(
        const izenelib::util::UString& text,
        const std::string& propertyName,
        unsigned int propertyId,
        const AnalysisInfo& analysisInfo)
{
    CREATE_SCOPED_PROFILER(proTermExtracting, "IndexTaskService", "Analyzer overhead");

//    la::TermIdList termIdList;
    laInputs_[propertyId]->resize(0);

    // Remove the spaces between two Chinese Characters
//    izenelib::util::UString refinedText;
//    la::removeRedundantSpaces( text, refinedText );
//    if (laManager_->getTermList(refinedText, analysisInfo, true, termList, true ) == false)
    la::MultilangGranularity indexingLevel = bundleConfig_->indexMultilangGranularity_;
    if (indexingLevel == la::SENTENCE_LEVEL)
    {
        if(bundleConfig_->bIndexUnigramProperty_)
        {
            if(propertyName.find("_unigram") != std::string::npos)
                indexingLevel = la::FIELD_LEVEL;  /// for unigram property, we do not need sentence level indexing
        }
    }

    if (laManager_->getTermIdList(idManager_.get(), text, analysisInfo, (*laInputs_[propertyId]), indexingLevel) == false)
            return false;

    return true;
}

bool WorkerService::makeSentenceBlocks_(
        const izenelib::util::UString & text,
        const unsigned int maxDisplayLength,
        const unsigned int numOfSummary,
        vector<CharacterOffset>& sentenceOffsetList)
{
    sentenceOffsetList.clear();
    if (summarizer_.getOffsetPairs(text, maxDisplayLength, numOfSummary, sentenceOffsetList) == false)
    {
        return false;
    }
    return true;
}


}
