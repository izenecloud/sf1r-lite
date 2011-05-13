#include "IndexTaskService.h"
#include "IndexBundleHelper.h"

#include <bundles/mining/MiningTaskService.h>
#include <bundles/recommend/RecommendTaskService.h>

#include <common/SFLogger.h>
#include <common/Utilities.h>
#include <document-manager/DocumentLog.h>
#include <license-manager/LicenseManager.h>

#include <util/profiler/ProfilerGroup.h>

#include <boost/filesystem.hpp>

#include <glog/logging.h>

#include <memory> // for auto_ptr
#include <signal.h>
#include <protect/RestrictMacro.h>
namespace bfs = boost::filesystem;

using namespace izenelib::driver;

namespace sf1r
{
IndexTaskService::IndexTaskService(
    IndexBundleConfiguration* bundleConfig, 
    DirectoryRotator& directoryRotator,
    boost::shared_ptr<IndexManager> indexManager)
    : bundleConfig_(bundleConfig)
    , directoryRotator_(directoryRotator)
    , miningTaskService_(NULL)
    , recommendTaskService_(NULL)
    , indexManager_(indexManager)
    , collectionId_(1)
    , maxDocId_(0)
    , indexProgress_()
    , checkInsert_(false)
    , numDeletedDocs_(0)
    , numUpdatedDocs_(0)
{
    std::set<PropertyConfig, PropertyComp>::iterator piter;
    bool hasDateInConfig = false;
    for (piter = bundleConfig_->schema_.begin();
            piter != bundleConfig_->schema_.end(); ++piter)
    {
        std::string propertyName = piter->getName();
        boost::to_lower(propertyName);
        if (propertyName=="date")
        {
            hasDateInConfig = true;
            dateProperty_ = *piter;
            break;
        }
    }
    if (!hasDateInConfig)
        throw std::runtime_error("Date Property Doesn't exist in config");

    indexStatus_.numDocs_ = indexManager_->getIndexReader()->numDocs();

    ///propertyId starts from 1
    laInputs_.resize(bundleConfig_->schema_.size() + 1);

    typedef IndexBundleSchema::iterator prop_iterator;
    for (prop_iterator iter = bundleConfig_->schema_.begin(),
                    iterEnd = bundleConfig_->schema_.end();
         iter != iterEnd; ++iter)
    {
        boost::shared_ptr<LAInput> laInput(new LAInput);
        laInputs_[iter->getPropertyId()] = laInput;
    }

}

IndexTaskService::~IndexTaskService()
{
}


bool IndexTaskService::buildCollection(unsigned int numdoc)
{
    string scdPath = bundleConfig_->collPath_.getScdPath() + "index/";

    if(!backup_() )
    {
        std::cout<<"backup failed."<<std::endl;
        return false;
    }
    
    DirectoryGuard dirGuard(directoryRotator_.currentDirectory().get());
    if (!dirGuard)
    {
        sflog->error( SFL_IDX, 10303);
        return false;
    }
    
    sf1r::Status::Guard statusGuard(indexStatus_);
    CREATE_PROFILER(buildIndex, "Index:SIAProcess", "Indexer : buildIndex")

    START_PROFILER(buildIndex);

    DLOG(INFO) << "start BuildCollection"<<endl;

    sflog->info(SFL_IDX, 10101);

    izenelib::util::ClockTimer timer;

    indexProgress_.reset();

    ScdParser parser(bundleConfig_->encoding_);

    // saves the name of the scd files in the path
    vector<string> scdList;
    try
    {
        if (bfs::is_directory(scdPath) == false)
        {
            sflog->error(SFL_SCD, 10102, scdPath.c_str() );
            return false;
        }
    }
    catch(boost::filesystem::filesystem_error& e)
    {
        std::cerr << e.what() << std::endl;
        sflog->error(SFL_IDX, 10137, e.what() );
        return false;
    }


    // search the directory for files
    static const bfs::directory_iterator kItrEnd;
    for (bfs::directory_iterator itr(scdPath); itr != kItrEnd; ++itr)
    {
        std::string fileName = itr->path().filename();

        if (parser.checkSCDFormat(fileName) )
        {
            scdList.push_back(itr->path().string() );
        }
        else
        {
            sflog->warn(SFL_SCD, 10103, fileName.c_str() );
        }
        parser.load(scdPath+fileName);
        indexProgress_.totalFileSize_ += parser.getFileSize();
    }

    indexProgress_.totalFileNum = scdList.size();

    if (indexProgress_.totalFileNum == 0)
    {
        sflog->warn(SFL_SCD, 10104, scdPath.c_str() );
        return false;
    }

    indexProgress_.currentFileIdx = 1;

    //sort scdList
    sort(scdList.begin(), scdList.end(), ScdParser::compareSCD);

    sflog->info(SFL_SCD, 10135, scdPath.c_str());
    vector<string>::iterator scd_it;
    for ( scd_it = scdList.begin(); scd_it != scdList.end(); ++scd_it)
        sflog->info(SFL_SCD, 10136, boost::filesystem::path(*scd_it).stem().c_str());

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

            sflog->info( SFL_SCD, 10105, boost::filesystem::path(*scd_it).stem().c_str() );

            switch ( parser.checkSCDType(*scd_it) )
            {
            case INSERT_SCD:
            {
                if (doBuildCollection_( *scd_it, 1, numdoc ) == false)
                {
                    //continue;
                }
                sflog->info( SFL_IDX, 10129);

            }
            break;
            case DELETE_SCD:
            {
                if (documentManager_->getMaxDocId()> 0)
                {
                    doBuildCollection_( *scd_it, 3, 0 );
                    sflog->info( SFL_IDX, 10131);
                }
                else
                {
                    sflog->warn( SFL_SCD, 10106,
                                 boost::filesystem::path(*scd_it).stem().c_str() );
                }
            }
            break;
            case UPDATE_SCD:
            {
                if (documentManager_->getMaxDocId()> 0)
                {
                    doBuildCollection_( *scd_it, 2, 0 );
                    sflog->info( SFL_IDX, 10130);
                }
                else
                {
                    sflog->warn( SFL_SCD, 10106, boost::filesystem::path(*scd_it).stem().c_str() );
                }
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
        //debug documentmanager's performance
//         std::cout<<"start debug documentmanager..."<<std::endl;
//         izenelib::util::ClockTimer clocker;
//         for(uint32_t i=1;i<documentManager_->getMaxDocId();i++)
//         {
//           if( i%1000 == 0 )
//           {
//             std::cout<<"debuging "<<i<<std::endl;
//           }
//           Document doc;
//           documentManager_->getDocument(i, doc);
//         }
//         std::cout<<"end debug documentmanager, cost "<<clocker.elapsed()<<std::endl;
        
        idManager_->flush();
        indexManager_->flush();
        //indexManager_->optimizeIndex();
        if( bundleConfig_->isTrieWildcard()) {
#ifdef __x86_64
            idManager_->startWildcardProcess();
            idManager_->joinWildcardProcess();
#endif
        }

        if( miningTaskService_ )
        {
          indexManager_->pauseMerge();
          miningTaskService_->DoMiningCollection();
          indexManager_->resumeMerge();
        }
        // Will be called by "commands/index_recommend"
//        if( recommendTaskService_ )
//        {
//            indexManager_->pauseMerge();
//            recommendTaskService_->buildCollection();
//            indexManager_->resumeMerge();
//        }
    }
    catch (std::exception& e)
    {
        indexProgress_.getIndexingStatus(indexStatus_);
        indexProgress_.reset();
//         documentManager_->flush();
//         idManager_->flush();
//         indexManager_->flush();
//         miningManager_->flush();
        return false;
    }
    indexProgress_.getIndexingStatus(indexStatus_);
    sflog->info(SFL_IDX, 10109,
                documentManager_->getMaxDocId(), numDeletedDocs_, numUpdatedDocs_) ;

    //both variables are refreshed
    numDeletedDocs_ = 0;
    numUpdatedDocs_ = 0;

    indexProgress_.reset();
    REPORT_PROFILE_TO_FILE( "PerformanceIndexResult.SIAProcess" )
    DLOG(INFO) << "End BuildCollection: "<< endl;
    DLOG(INFO) << "time elapsed:" << timer.elapsed() <<"seconds\n";

    STOP_PROFILER(buildIndex);
    return true;
}

bool IndexTaskService::optimizeIndex()
{
    if(!backup_() )
    {
        std::cout<<"backup failed."<<std::endl;
        return false;
    }
    
    DirectoryGuard dirGuard(directoryRotator_.currentDirectory().get());
    if (!dirGuard)
    {
        sflog->error( SFL_IDX, 10303);
        return false;
    }
    indexManager_->optimizeIndex();
    return true;
}

bool IndexTaskService::createDocument(const Value& documentValue)
{
    if(!backup_() )
    {
        std::cout<<"backup failed."<<std::endl;
        return false;
    }
    
    DirectoryGuard dirGuard(directoryRotator_.currentDirectory().get());
    if (!dirGuard)
    {
        sflog->error( SFL_IDX, 10303);
        return false;
    }
    sf1r::Status::Guard statusGuard(indexStatus_);

    maxDocId_ = documentManager_->getMaxDocId();
    if ( LicenseManager::continueIndex_ )
    {
        COBRA_RESTRICT_EXCEED_N_RETURN_FALSE(maxDocId_, LICENSE_MAX_DOC,  0);
    }
    else
    {
        COBRA_RESTRICT_EXCEED_N_RETURN_FALSE(maxDocId_, LicenseManager::TRIAL_MAX_DOC,  0);
    }

    SCDDoc scddoc;
    value2SCDDoc(documentValue, scddoc);

    Document document;
    IndexerDocument indexDocument;
    bool rType = false;
    std::map<std::string, pair<PropertyDataType, izenelib::util::UString> > rTypeFieldValue;
    sf1r::docid_t id = 0;
    if (!prepareDocument_(scddoc, document, indexDocument, rType, rTypeFieldValue))
    {
        return false;
    }

    if (rType)
    {
        id = document.getId();
    }

    // DEBUG
    static int docnum = 1;
    fprintf(stderr, "\n-----[%-6d] Document is inserted\n", docnum++);
    // DEBUG

    if (documentManager_->insertDocument(document))
    {
        indexManager_->insertDocument(indexDocument);
        indexStatus_.numDocs_ = indexManager_->getIndexReader()->numDocs();
        searchManager_->reset_cache(rType, id, rTypeFieldValue);
        return true;
    }

    return false;
}

bool IndexTaskService::updateDocument(const Value& documentValue)
{
    if(!backup_() )
    {
        std::cout<<"backup failed."<<std::endl;
        return false;
    }
    
    DirectoryGuard dirGuard(directoryRotator_.currentDirectory().get());
    if (!dirGuard)
    {
        sflog->error( SFL_IDX, 10303);
        return false;
    }
    sf1r::Status::Guard statusGuard(indexStatus_);
    SCDDoc scddoc;
    value2SCDDoc(documentValue, scddoc);

    Document document;
    IndexerDocument indexDocument;
    bool rType = false;
    std::map<std::string, pair<PropertyDataType, izenelib::util::UString> > rTypeFieldValue;
    sf1r::docid_t id = 0;
    if (!prepareDocument_(scddoc, document, indexDocument, rType, rTypeFieldValue, false))
    {
        return false;
    }

    if (rType)
    {
        id = document.getId();
    }

    sf1r::docid_t oldId = indexDocument.getId();
    if(oldId == 0)
    {
        /// document does not exist, directly index
        if (documentManager_->insertDocument(document))
        {
            indexManager_->insertDocument(indexDocument);
            indexStatus_.numDocs_ = indexManager_->getIndexReader()->numDocs();
            searchManager_->reset_cache(rType, id, rTypeFieldValue);
            return true;
        }
    }
    else
    {
        if (!rType)
        {
            if (!documentManager_->removeDocument(oldId))
            {
                 return false;
            }

            // Insert document data to the SDB repository.
             if (documentManager_->insertDocument(document) )
             {
                 indexManager_->updateDocument(indexDocument);
                 ++numUpdatedDocs_;
                 searchManager_->reset_cache(rType, id, rTypeFieldValue);
                 return true;
             }
        }
        else
        {
            // Store the old property value.
            IndexerDocument oldIndexDocument;
            if ( !preparePartialDocument_(document, oldIndexDocument) )
            {
                return false;
            }

            // Update document data in the SDB repository.
            if ( documentManager_->updatePartialDocument(document) )
            {
                indexManager_->updateRtypeDocument(oldIndexDocument, indexDocument);
                ++numUpdatedDocs_;
                searchManager_->reset_cache(rType, id, rTypeFieldValue);
                return true;
            }
        }
    }
    return false;
}

bool IndexTaskService::preparePartialDocument_(Document& document, IndexerDocument& oldIndexDocument)
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
        if (it->first != "DOCID" && it->first != "DATE") {
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
            indexerPropertyConfig.setPropertyId(iter->getPropertyId());
            indexerPropertyConfig.setName(iter->getName());
            indexerPropertyConfig.setIsIndex(iter->isIndex());
            indexerPropertyConfig.setIsAnalyzed(iter->isAnalyzed());
            indexerPropertyConfig.setIsFilter(iter->getIsFilter());
            indexerPropertyConfig.setIsMultiValue(iter->getIsMultiValue());
            indexerPropertyConfig.setIsStoreDocLen(iter->getIsStoreDocLen());

            PropertyValue value = oldDoc.property(it->first);
            const izenelib::util::UString* stringValue = get<izenelib::util::UString>(&value);

            std::string str("");
            stringValue->convertString(str, bundleConfig_->encoding_);
            if ( iter->getType() == INT_PROPERTY_TYPE )
            {
                int64_t value = 0;
                value = boost::lexical_cast< int64_t >( str );
                oldIndexDocument.insertProperty(indexerPropertyConfig, value);
            }
            else if ( iter->getType() == UNSIGNED_INT_PROPERTY_TYPE )
            {
                uint64_t value = 0;
                value = boost::lexical_cast< uint64_t >( str );
                oldIndexDocument.insertProperty(indexerPropertyConfig, value);
            }
            else if ( iter->getType() == FLOAT_PROPERTY_TYPE )
            {
                float value = 0.0;
                value = boost::lexical_cast< float >( str );
                oldIndexDocument.insertProperty(indexerPropertyConfig, value);
            }
            else if ( iter->getType() == DOUBLE_PROPERTY_TYPE )
            {
                 double value = 0.0;
                 value = boost::lexical_cast< double >( str );
                 oldIndexDocument.insertProperty(indexerPropertyConfig, value);
            }
        }
    }
    return true;
}

bool IndexTaskService::destroyDocument(const Value& documentValue)
{
    if(!backup_() )
    {
        std::cout<<"backup failed."<<std::endl;
        return false;
    }
    
    DirectoryGuard dirGuard(directoryRotator_.currentDirectory().get());
    if (!dirGuard)
    {
        sflog->error( SFL_IDX, 10303);
        return false;
    }
    sf1r::Status::Guard statusGuard(indexStatus_);
    sf1r::docid_t docid;
    izenelib::util::UString docName(asString(documentValue["DOCID"]),
                             izenelib::util::UString::UTF_8);
    bool ret = idManager_->getDocIdByDocName(docName, docid, false);
    std::map<std::string, pair<PropertyDataType, izenelib::util::UString> > rTypeFieldValue;

    if(!ret)
    {
        string property;
        docName.convertString(property, bundleConfig_->encoding_ );
        sflog->error(SFL_IDX,10142, property.c_str());
    }
    if (documentManager_->removeDocument(docid))
    {
        indexManager_->removeDocument(collectionId_, docid);
        ++numDeletedDocs_;
        indexStatus_.numDocs_ = indexManager_->getIndexReader()->numDocs();
        searchManager_->reset_cache(false, 0, rTypeFieldValue);

        return true;
    }

    return false;
}

bool IndexTaskService::doBuildCollection_(const std::string& fileName, int op, uint32_t numdoc)
{
    CREATE_PROFILER(proDocumentIndexing, "Index:SIAProcess", "Indexer : InsertDocument")
    CREATE_PROFILER(proIndexing, "Index:SIAProcess", "Indexer : indexing")
    CREATE_PROFILER(proMining, "Index:SIAProcess", "Indexer : mining")

    ScdParser parser(bundleConfig_->encoding_);
    if (parser.load(fileName) == false)
    {
        sflog->error(SFL_SCD, 10110, fileName.c_str() );
        return false;
    }
    indexProgress_.currentFileSize_ = parser.getFileSize();
    indexProgress_.currentFilePos_ = 0;
    if( op <= 2 ) // insert or update
    {
        config_tool::buildPropertyAliasMap(bundleConfig_->schema_,
                                       propertyAliasMap_);
        uint32_t n = 0;
        long lastOffset = 0;
        bool isInsert = (op == 1)?true:false;
        for (ScdParser::iterator doc_iter = parser.begin(); doc_iter
                != parser.end(); ++doc_iter, ++n)
        {
            if (*doc_iter == NULL)
            {
                sflog->warn(SFL_SCD, 10103,
                            boost::filesystem::path(fileName).stem().c_str() );
                return false;
            }

            indexProgress_.currentFilePos_ += doc_iter.getOffset() - lastOffset;
            indexProgress_.totalFilePos_ += doc_iter.getOffset() - lastOffset;
            lastOffset = doc_iter.getOffset();
            if ( (0 < numdoc) && (numdoc <= n))
                break;

            if (n%1000 == 0)
            {
                indexProgress_.getIndexingStatus(indexStatus_);
                indexStatus_.progress_ = indexProgress_.getTotalPercent();
                indexStatus_.elapsedTime_ = boost::posix_time::seconds((int)indexProgress_.getElapsed());
                indexStatus_.leftTime_ =  boost::posix_time::seconds((int)indexProgress_.getLeft());
            }

            SCDDocPtr doc = (*doc_iter);
            Document document;
            IndexerDocument indexDocument;
            bool rType = false;
            std::map<std::string, pair<PropertyDataType, izenelib::util::UString> > rTypeFieldValue;
            sf1r::docid_t id = 0;
            bool ret = prepareDocument_( *doc, document, indexDocument, rType, rTypeFieldValue, isInsert);//or is update

            if ( !ret)
                continue;

            if (rType)
            {
                id = document.getId();
            }
            
            uint32_t oldId = 0;
            if( !isInsert )
            {
                oldId = indexDocument.getId();
                if( oldId == 0 ) isInsert = true;
            }
            // begin insert and update document
            if( isInsert )
            {
                START_PROFILER( proDocumentIndexing );
                if (documentManager_->insertDocument(document) == false)
                {
                    sflog->error(SFL_IDX, 10112,boost::filesystem::path(fileName).stem().c_str() );
                    continue;
                }
                STOP_PROFILER( proDocumentIndexing );

                START_PROFILER( proIndexing );
                indexManager_->insertDocument(indexDocument);
                STOP_PROFILER( proIndexing );
				   
                indexStatus_.numDocs_ = indexManager_->getIndexReader()->numDocs();
            }
            else
            {
                if (!rType)
                {
                    if (!documentManager_->removeDocument(oldId))
                    {
                        sflog->warn(SFL_SCD, 10116, oldId);
                        continue;
                    }
                    if (documentManager_->insertDocument(document) == false)
                    {
                         sflog->error(SFL_IDX, 10112,
                                      boost::filesystem::path(fileName).stem().c_str() );
                         continue;
                    }

                    indexManager_->updateDocument(indexDocument);
                }
                else
                {
                    // Store the old property value.
                    IndexerDocument oldIndexDocument;
                    if ( !preparePartialDocument_(document, oldIndexDocument) )
                    {
                        return false;
                    }

                    // Update document data in the SDB repository.
                    if ( documentManager_->updatePartialDocument(document) == false )
                    {
                         sflog->error(SFL_IDX, 10112,
                                      boost::filesystem::path(fileName).stem().c_str() );
                         continue;
                    }

                    indexManager_->updateRtypeDocument(oldIndexDocument, indexDocument);
                 }
                ++numUpdatedDocs_;
            }
//             uint32_t nn = n%200;
//             if( nn == 0 )
//             {
//               std::cout<<"Collection Builder : added "<<n<<" docs."<<std::endl;
//             }
            searchManager_->reset_cache(rType, id, rTypeFieldValue);

            // interrupt when closing the process
            boost::this_thread::interruption_point();

        } // end of for loop for all documents
    }
    else //delete
    {
        std::vector<izenelib::util::UString> rawDocIDList;
        if (parser.getDocIdList(rawDocIDList) == false)
        {
            sflog->warn(SFL_SCD, 10103,
                        boost::filesystem::path(fileName).stem().c_str() );

            return false;
        }
        //get the docIds for deleting
        std::vector<sf1r::docid_t> docIdList;
        docIdList.reserve(rawDocIDList.size());
        indexProgress_.currentFileSize_ =rawDocIDList.size();
        indexProgress_.currentFilePos_ = 0;
        for (std::vector<izenelib::util::UString>::iterator iter = rawDocIDList.begin(); iter
                != rawDocIDList.end(); ++iter)
        {
            sf1r::docid_t docId;
            bool ret = idManager_->getDocIdByDocName(*iter, docId, false);
            if(ret)
                docIdList.push_back(docId);
            else
            {
                string property;
                iter->convertString(property, bundleConfig_->encoding_ );
                sflog->error(SFL_IDX,10142, property.c_str());
            }
        }

        //process delete document in index manager
        for (std::vector<sf1r::docid_t>::iterator iter = docIdList.begin(); iter
                != docIdList.end(); ++iter)
        {
            if (numDeletedDocs_%1000 == 0)
            {
                indexProgress_.getIndexingStatus(indexStatus_);
                indexStatus_.progress_ = indexProgress_.getTotalPercent();
                indexStatus_.elapsedTime_ = boost::posix_time::seconds((int)indexProgress_.getElapsed());
                indexStatus_.leftTime_ =  boost::posix_time::seconds((int)indexProgress_.getLeft());
            }
            //marks delete key to true in DB
            if (!documentManager_->removeDocument(*iter))
            {
                sflog->warn(SFL_SCD, 10116, *iter);
                continue;
            }
            ++indexProgress_.currentFilePos_;
            indexManager_->removeDocument(collectionId_, *iter);
            ++numDeletedDocs_;
            indexStatus_.numDocs_ = indexManager_->getIndexReader()->numDocs();
        }

        std::map<std::string, pair<PropertyDataType, izenelib::util::UString> > rTypeFieldValue;
        searchManager_->reset_cache(false, 0, rTypeFieldValue);

        // interrupt when closing the process
        boost::this_thread::interruption_point();
        
    }

    return true;
}

void IndexTaskService::checkRtype_(SCDDoc& doc, bool& rType,
        std::map<std::string, pair<PropertyDataType, izenelib::util::UString> >& rTypeFieldValue)
{
    //R-type check
    PropertyDataType dataType;
    vector<pair<izenelib::util::UString, izenelib::util::UString> >::iterator p;
    for (p = doc.begin(); p != doc.end(); p++)
    {
        std::set<PropertyConfig, PropertyComp>::iterator iter;
        string fieldName;
        p->first.convertString(fieldName, bundleConfig_->encoding_ );

        PropertyConfig tempPropertyConfig;
        tempPropertyConfig.propertyName_ = fieldName;
        iter = bundleConfig_->schema_.find(tempPropertyConfig);

        if ( iter != bundleConfig_->schema_.end() )
        {
            if ( iter->getName() == "DOCID" || iter->getName() == "DATE" )
            {
                continue;
            }
            if ( iter-> isIndex() && iter->getIsFilter() && !iter->isAnalyzed())
            {
                dataType = iter->getType();
                if ( dataType != INT_PROPERTY_TYPE && dataType != UNSIGNED_INT_PROPERTY_TYPE
                    && dataType != FLOAT_PROPERTY_TYPE && dataType != DOUBLE_PROPERTY_TYPE )
                {
                    break;
                }
                pair<PropertyDataType, izenelib::util::UString> fieldValue;
                fieldValue.first = dataType;
                fieldValue.second = p->second;
                rTypeFieldValue[iter->getName()] = fieldValue;
            }
            else
            {
                break;
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
}

bool IndexTaskService::prepareDocument_(SCDDoc& doc, Document& document,
                                    IndexerDocument& indexDocument, bool& rType,
                                    std::map<std::string, pair<PropertyDataType, izenelib::util::UString> >& rTypeFieldValue,
                                    bool insert)
{
    sf1r::docid_t docId = 0;
    sf1r::docid_t Id = 0;///old doc id for update
    string fieldStr;
    vector<CharacterOffset> sentenceOffsetList;
    AnalysisInfo analysisInfo;
    // the iterator is not const because the p-second value may change
    // due to the maxlen setting
    vector<pair<izenelib::util::UString, izenelib::util::UString> >::iterator p;
    bool dateExistInSCD = false;

    for (p = doc.begin(); p != doc.end(); p++)
    {
        bool extraProperty = false;
        std::set<PropertyConfig, PropertyComp>::iterator iter;
        p->first.convertString(fieldStr, bundleConfig_->encoding_ );

        PropertyConfig temp;
        temp.propertyName_ = fieldStr;
        iter = bundleConfig_->schema_.find(temp);

        IndexerPropertyConfig indexerPropertyConfig;
        if (iter == bundleConfig_->schema_.end())
            extraProperty = true;

        izenelib::util::UString propertyNameL = p->first;
        propertyNameL.toLowerString();
        const izenelib::util::UString & propertyValueU = p->second; // preventing copy

        if (!extraProperty)
        {
            indexerPropertyConfig.setPropertyId(iter->getPropertyId());
            indexerPropertyConfig.setName(iter->getName());
            indexerPropertyConfig.setIsIndex(iter->isIndex());
            indexerPropertyConfig.setIsAnalyzed(iter->isAnalyzed());
            indexerPropertyConfig.setIsFilter(iter->getIsFilter());
            indexerPropertyConfig.setIsMultiValue(iter->getIsMultiValue());
            indexerPropertyConfig.setIsStoreDocLen(iter->getIsStoreDocLen());
        }

        izenelib::util::UString::EncodingType encoding = bundleConfig_->encoding_;
        if ( (propertyNameL == izenelib::util::UString("docid", encoding) )
                && (!extraProperty))
        {
            if(insert)
            {
                bool ret = idManager_->getDocIdByDocName(propertyValueU, docId);
                if (ret)
                {
                    string scdDocId;
                    propertyValueU.convertString(scdDocId, encoding );
                    sflog->warn(SFL_IDX, 10125, docId, scdDocId.c_str());
                    return false;
                }
                if (docId <= maxDocId_)
                {
                    sflog->warn(SFL_IDX,10124, docId);
                    return false;
                }
            }
            else
            {
                //R-type check
                checkRtype_(doc, rType, rTypeFieldValue);
                if ( !rType )
                {
                    idManager_->updateDocIdByDocName(propertyValueU, Id, docId);
                }
                else
                {
                    idManager_->getDocIdByDocName(propertyValueU, Id, false);
                    docId = Id;
                }
            }

            document.setId(docId);
            document.property(fieldStr) = propertyValueU;
            indexDocument.setId(Id);
            indexDocument.setDocId(docId, collectionId_);
        }
        else if (propertyNameL == izenelib::util::UString("date", encoding) )
        {
            /// format <DATE>20091009163011
            dateExistInSCD = true;
            izenelib::util::UString dateStr;
            int64_t time = sf1r::Utilities::convertDate(propertyValueU, encoding, dateStr);
            indexerPropertyConfig.setPropertyId(dateProperty_.getPropertyId());
            indexerPropertyConfig.setName(dateProperty_.getName());
            indexerPropertyConfig.setIsIndex(true);
            indexerPropertyConfig.setIsFilter(true);
            indexerPropertyConfig.setIsAnalyzed(false);
            indexerPropertyConfig.setIsMultiValue(false);
            indexDocument.insertProperty(indexerPropertyConfig, time);
            document.property(dateProperty_.getName()) = dateStr;
        }
        else if (!extraProperty)
        {
            if (iter->getType() == STRING_PROPERTY_TYPE)
            {
                ///process for properties that requires forward index to be created
                if ( (iter->isIndex() == true) && (!extraProperty))
                {
                    analysisInfo.clear();
                    analysisInfo = iter->getAnalysisInfo();
                    if (analysisInfo.analyzerId_.size() == 0)
                    {
                        document.property(fieldStr) = propertyValueU;
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
                        if (makeForwardIndex_(propertyValueU, iter->getPropertyId(), analysisInfo) == false)
                        {
                            DLOG(ERROR) << "Forward Indexing Failed Error Line : "<< __LINE__ << endl;
                            return false;
                        }
                        if (iter->getIsFilter())
                        {
                            if(iter->getIsMultiValue())
                            {
                                MultiValuePropertyType props;
                                split_string(propertyValueU,props, encoding,',');

                                MultiValueIndexPropertyType
                                indexData = std::make_pair(laInputs_[iter->getPropertyId()],props );
                                indexDocument.insertProperty(indexerPropertyConfig, indexData);
                            
                            }
                            else
                            {
                                IndexPropertyType
                                indexData =
                                    std::make_pair(
                                        laInputs_[iter->getPropertyId()],
                                        const_cast<izenelib::util::UString &>(propertyValueU) );
                                indexDocument.insertProperty(indexerPropertyConfig, indexData);
                            }
                        }
                        else
                            indexDocument.insertProperty(
                                indexerPropertyConfig, laInputs_[iter->getPropertyId()]);


                        document.property(fieldStr) = propertyValueU;
                        unsigned int numOfSummary = 0;
                        if ( (iter->getIsSnippet() == true)
                                || (iter->getIsSummary() == true))
                        {
                            if (iter->getIsSummary() == true)
                            {
                                numOfSummary = iter->getSummaryNum();
                                if (numOfSummary <= 0)
                                    numOfSummary = 1; //atleast one sentence required for summary
                            }

                            if (makeSentenceBlocks_(propertyValueU, iter->getPropertyId(), iter->getDisplayLength(),
                                                    numOfSummary, sentenceOffsetList) == false)
                            {
                                sflog->error(SFL_IDX, 10129);
                            }

                            document.property(fieldStr + ".blocks")
                            = sentenceOffsetList;
                        }


                        // For alias indexing
                        config_tool::PROPERTY_ALIAS_MAP_T::iterator mapIter =
                            propertyAliasMap_.find(iter->getName() );
                        if (mapIter != propertyAliasMap_.end() ) // if there's alias property
                        {
                            std::vector<PropertyConfig>::iterator vecIter =
                                mapIter->second.begin();
                            for (; vecIter != mapIter->second.end(); vecIter++)
                            {
                                AnalysisInfo aliasAnalysisInfo =
                                    vecIter->getAnalysisInfo();
                                laInputs_[vecIter->getPropertyId()]->setDocId(docId);
                                if (makeForwardIndex_(propertyValueU,
                                                      vecIter->getPropertyId(),
                                                      aliasAnalysisInfo) == false)
                                {
                                    DLOG(ERROR) << "Forward Indexing Failed Error Line : "<< __LINE__ << endl;
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
                        } // end - if ( mapIter != end() )

                    }
                }
                // insert property name and value for other properties that is not DOCID and neither required to be indexed
                else
                {
                    //insert only if property that exist in collection configuration
                    if (!extraProperty)
                    {
                        //extra properties that need not be indexed to be stored only in document manager
                        document.property(fieldStr) = propertyValueU;
                        //other extra properties that need not be in index manager
                        indexDocument.insertProperty(indexerPropertyConfig,
                                                      propertyValueU);
                    }
                }
            }
            else if (iter->getType() == INT_PROPERTY_TYPE)
            {
                document.property(fieldStr) = propertyValueU;
                if ( (iter->isIndex() == true) && (!extraProperty))
                {
                    if(iter->getIsMultiValue())
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
                            value = boost::lexical_cast< int64_t >( str );
                        }
                        catch( const boost::bad_lexical_cast & )
                        {
                            sflog->error(SFL_IDX,10140, docId);
                        }
                        indexDocument.insertProperty(indexerPropertyConfig, value);
                    }
                }
            }
            else if (iter->getType() == FLOAT_PROPERTY_TYPE)
            {
                document.property(fieldStr) = propertyValueU;
                if ( (iter->isIndex() == true) && (!extraProperty))
                {
                    if(iter->getIsMultiValue())
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
                            value = boost::lexical_cast< float >( str );
                        }
                        catch( const boost::bad_lexical_cast & )
                        {
                            sflog->error(SFL_IDX,10140, docId);
                        }
                        indexDocument.insertProperty(indexerPropertyConfig, value);
                    }
                }
            }
            else if (iter->getType() == NOMINAL_PROPERTY_TYPE)
            {
                document.property(fieldStr) = propertyValueU;

            }
            else
            {
            }
        }
    }
    if (!dateExistInSCD)
    {
        IndexerPropertyConfig indexerPropertyConfig;
        indexerPropertyConfig.setPropertyId(dateProperty_.getPropertyId());
        indexerPropertyConfig.setName(dateProperty_.getName());
        indexerPropertyConfig.setIsIndex(true);
        indexerPropertyConfig.setIsFilter(true);
        indexerPropertyConfig.setIsAnalyzed(false);
        indexerPropertyConfig.setIsMultiValue(false);
        izenelib::util::UString dateStr;
        izenelib::util::UString emptyDateStr;
        int64_t time = sf1r::Utilities::convertDate(emptyDateStr, izenelib::util::UString::UTF_8, dateStr);
        indexDocument.insertProperty(indexerPropertyConfig, time);
        document.property(dateProperty_.getName()) = dateStr;
    }
    return true;
}

/// @desc Make a forward index of a given text.
/// You can specify an Language Analysis option through AnalysisInfo parameter.
/// You have to get a proper AnalysisInfo value from the configuration. (Currently not implemented.)
bool IndexTaskService::makeForwardIndex_(const izenelib::util::UString& text,
                           unsigned int propertyId,
                           const AnalysisInfo& analysisInfo)
{
    CREATE_PROFILER(proTermExtracting, "IndexTaskService:SIAProcess",
                    "Forward Index Building : extracting Terms");
    CREATE_PROFILER(proRawMap, "IndexTaskService:SIAProcess",
                    "Forward Index Building : inserting raw map");
    CREATE_PROFILER(proTermIdGeneration, "IndexTaskService:SIAProcess",
                    "Forward Index Building : creating IDs");
    CREATE_PROFILER(proInsertOffset, "IndexTaskService:SIAProcess",
                    "Forward Index Building : insert offset in FI ");

//    la::TermIdList termIdList;
    laInputs_[propertyId]->resize(0);

    START_PROFILER(proTermExtracting);
    // Remove the spaces between two Chinese Characters
//    izenelib::util::UString refinedText;
//    la::removeRedundantSpaces( text, refinedText );
//    if (laManager_->getTermList(refinedText, analysisInfo, true, termList, true ) == false)

    if (laManager_->getTermIdList(idManager_.get(), text, analysisInfo, (*laInputs_[propertyId]) ) == false)
    {
        //DocumentLog::print(DocumentLog::TYPE_ERROR, "", "getTermsByKoreanLanguageExtractor() failed.");
        return false;
    }
    STOP_PROFILER(proTermExtracting);
/*
    unsigned int id = 0;

    for (la::TermIdList::iterator p = termIdList.begin(); p != termIdList.end(); p++)
    {
        //TODO:substitute p->byteOffset instead of 0 for ensuring range of Word offsets for the terms processed through regulators
        START_PROFILER(proTermIdGeneration);
//        idManager_->getTermIdByTermString(p->text_, id);

        //std::cout<<",";p->text_.displayStringInfo( izenelib::util::UString::UTF_8);
//        /// TODO, disable if the collection uses unigram wildcard
//        if( collectionMeta_.isTrieWildcard() ) {
//            idManager_->addWildcardCandidate(p->text_);
//        }

        STOP_PROFILER(proTermIdGeneration);
        START_PROFILER(proInsertOffset);
        laInput->push_back(LAInputUnit(p->termid_, p->wordOffset_));
        STOP_PROFILER(proInsertOffset);
    }//std::cout<<std::endl<<std::endl;
*/
    return true;
}

bool IndexTaskService::makeSentenceBlocks_(const izenelib::util::UString & text,
                                       unsigned int propertyId,
                                       const unsigned int maxDisplayLength,
                                       const unsigned int numOfSummary,
                                       vector<CharacterOffset>& sentenceOffsetList)
{
    sentenceOffsetList.clear();
    if (summarizer_.getOffsetPairs(text, *laInputs_[propertyId], maxDisplayLength, numOfSummary,
                                   sentenceOffsetList) == false)
    {
        return false;
    }
    return true;
}

void IndexTaskService::value2SCDDoc(const Value& value, SCDDoc& scddoc)
{
    const Value::ObjectType& objectValue = value.getObject();
    scddoc.resize(objectValue.size());

    std::size_t propertyId = 0;
    for (Value::ObjectType::const_iterator it = objectValue.begin();
         it != objectValue.end(); ++it, ++propertyId)
    {
        scddoc[propertyId].first.assign(it->first, izenelib::util::UString::UTF_8);
        scddoc[propertyId].second.assign(
            asString(it->second),
            izenelib::util::UString::UTF_8
        );
    }
}

bool IndexTaskService::backup_()
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
            std::cout << "Copy index dir from "
                      << current->name()
                      << " to "
                      << next->name()
                      << std::endl;
            next->copyFrom(*current);
            return true;
        }
        catch(boost::filesystem::filesystem_error& e)
        {
            sflog->info( SFL_IDX, 10305, e.what() );
        }

        // try copying but failed
        return false;
    }

    // not copy, always returns true
    return true;
}

bool IndexTaskService::getIndexStatus(Status& status)
{
    indexProgress_.getIndexingStatus(indexStatus_);
    status = indexStatus_;
    return true;
}

}

