#include "IndexBundleActivator.h"
#include <bundles/mining/MiningSearchService.h>
#include <bundles/product/ProductSearchService.h>
#include <bundles/product/ProductTaskService.h>

#include <common/SFLogger.h>
#include <query-manager/QueryManager.h>
#include <index-manager/IndexManager.h>
#include <search-manager/SearchManager.h>
#include <search-manager/PersonalizedSearchInfo.h>
#include <ranking-manager/RankingManager.h>
#include <document-manager/DocumentManager.h>
#include <la-manager/LAManager.h>
#include <la-manager/LAPool.h>
#include <aggregator-manager/AggregatorManager.h>
#include <aggregator-manager/WorkerService.h>
#include <node-manager/MasterNodeManager.h>
#include <util/singleton.h>

#include <boost/filesystem.hpp>

#include <memory> // for auto_ptr

namespace bfs = boost::filesystem;
using namespace izenelib::util;

namespace sf1r
{

using namespace izenelib::osgi;
IndexBundleActivator::IndexBundleActivator()
    :miningSearchTracker_(0)
    ,miningTaskTracker_(0)
    ,recommendSearchTracker_(0)
    ,recommendTaskTracker_(0)
    ,productSearchTracker_(0)
    ,productTaskTracker_(0)
    ,context_(0)
    ,searchService_(0)
    ,searchServiceReg_(0)
    ,taskService_(0)
    ,taskServiceReg_(0)
    ,config_(0)
{
}

IndexBundleActivator::~IndexBundleActivator()
{
}

void IndexBundleActivator::start( IBundleContext::ConstPtr context )
{
    context_ = context;

    boost::shared_ptr<BundleConfiguration> bundleConfigPtr = context->getBundleConfig();
    config_ = static_cast<IndexBundleConfiguration*>(bundleConfigPtr.get());
    init_();

    Properties props;
    props.put( "collection", config_->collectionName_);
    searchServiceReg_ = context->registerService( "IndexSearchService", searchService_, props );
    taskServiceReg_ = context->registerService( "IndexTaskService", taskService_, props );
    miningSearchTracker_ = new ServiceTracker( context, "MiningSearchService", this );
    miningSearchTracker_->startTracking();
    miningTaskTracker_ = new ServiceTracker( context, "MiningTaskService", this );
    miningTaskTracker_->startTracking();

    productSearchTracker_ = new ServiceTracker( context, "ProductSearchService", this );
    productSearchTracker_->startTracking();
    productTaskTracker_ = new ServiceTracker( context, "ProductTaskService", this );
    productTaskTracker_->startTracking();

    recommendSearchTracker_ = new ServiceTracker( context, "RecommendSearchService", this );
    recommendSearchTracker_->startTracking();
    recommendTaskTracker_ = new ServiceTracker( context, "RecommendTaskService", this );
    recommendTaskTracker_->startTracking();
}

void IndexBundleActivator::stop( IBundleContext::ConstPtr context )
{
    indexManager_->flush();
    if(miningSearchTracker_)
    {
        miningSearchTracker_->stopTracking();
        delete miningSearchTracker_;
        miningSearchTracker_ = 0;
    }
    if(miningTaskTracker_)
    {
        miningTaskTracker_->stopTracking();
        delete miningTaskTracker_;
        miningTaskTracker_ = 0;
    }

    if(productSearchTracker_)
    {
        productSearchTracker_->stopTracking();
        delete productSearchTracker_;
        productSearchTracker_ = 0;
    }
    if(productTaskTracker_)
    {
        productTaskTracker_->stopTracking();
        delete productTaskTracker_;
        productTaskTracker_ = 0;
    }

    if(recommendSearchTracker_)
    {
        recommendSearchTracker_->stopTracking();
        delete recommendSearchTracker_;
        recommendSearchTracker_ = 0;
    }
    if(recommendTaskTracker_)
    {
        recommendTaskTracker_->stopTracking();
        delete recommendTaskTracker_;
        recommendTaskTracker_ = 0;
    }

    if(searchServiceReg_)
    {
        searchServiceReg_->unregister();
        delete searchServiceReg_;
        delete searchService_;
        searchServiceReg_ = 0;
        searchService_ = 0;
    }
    if(taskServiceReg_)
    {
        taskServiceReg_->unregister();
        delete taskServiceReg_;
        delete taskService_;
        taskServiceReg_ = 0;
        taskService_ = 0;
    }
}

bool IndexBundleActivator::addingService( const ServiceReference& ref )
{
    if ( ref.getServiceName() == "MiningSearchService" )
    {
        Properties props = ref.getServiceProperties();
        if ( props.get( "collection" ) == config_->collectionName_)
        {
            MiningSearchService* service = reinterpret_cast<MiningSearchService*> ( const_cast<IService*>(ref.getService()) );
            cout << "[IndexBundleActivator#addingService] Calling MiningSearchService..." << endl;
            searchService_->aggregatorManager_->miningManager_ = service->GetMiningManager();
            searchService_->workerService_->miningManager_ = service->GetMiningManager();
            return true;
        }
        else
        {
            return false;
        }
    }
    else if ( ref.getServiceName() == "MiningTaskService" )
    {
        Properties props = ref.getServiceProperties();
        if ( props.get( "collection" ) == config_->collectionName_)
        {
            MiningTaskService* service = reinterpret_cast<MiningTaskService*> ( const_cast<IService*>(ref.getService()) );
            cout << "[IndexBundleActivator#addingService] Calling MiningTaskService..." << endl;
            taskService_->miningTaskService_= service;
            return true;
        }
        else
        {
            return false;
        }
    }
    else if ( ref.getServiceName() == "ProductSearchService" )
    {
        Properties props = ref.getServiceProperties();
        if ( props.get( "collection" ) == config_->collectionName_)
        {
//             ProductSearchService* service = reinterpret_cast<ProductSearchService*> ( const_cast<IService*>(ref.getService()) );
            cout << "[IndexBundleActivator#addingService] Calling ProductSearchService..." << endl;
            // product index hook set in Product Bundle

            return true;
        }
        else
        {
            return false;
        }
    }
    else if ( ref.getServiceName() == "ProductTaskService" )
    {
        Properties props = ref.getServiceProperties();
        if ( props.get( "collection" ) == config_->collectionName_)
        {
            //ProductTaskService* service = reinterpret_cast<ProductTaskService*> ( const_cast<IService*>(ref.getService()) );
            cout << "[IndexBundleActivator#addingService] Calling ProductTaskService..." << endl;
            ///TODO
            return true;
        }
        else
        {
            return false;
        }
    }
    else if ( ref.getServiceName() == "RecommendSearchService" )
    {
        Properties props = ref.getServiceProperties();
        if ( props.get( "collection" ) == config_->collectionName_)
        {
            RecommendSearchService* service = reinterpret_cast<RecommendSearchService*> ( const_cast<IService*>(ref.getService()) );
            cout << "[IndexBundleActivator#addingService] Calling RecommendSearchService..." << endl;
            searchService_->workerService_->recommendSearchService_ = service;
            return true;
        }
        else
        {
            return false;
        }
    }
    else if ( ref.getServiceName() == "RecommendTaskService" )
    {
        Properties props = ref.getServiceProperties();
        if ( props.get( "collection" ) == config_->collectionName_)
        {
            RecommendTaskService* service = reinterpret_cast<RecommendTaskService*> ( const_cast<IService*>(ref.getService()) );
            cout << "[IndexBundleActivator#addingService] Calling RecommendTaskService..." << endl;
            taskService_->recommendTaskService_= service;
            return true;
        }
        else
        {
            return false;
        }
    }
    else
    {
        return false;
    }
}

void IndexBundleActivator::removedService( const ServiceReference& ref )
{

}

bool IndexBundleActivator::init_()
{
    std::cout<<"["<<config_->collectionName_<<"]"<<"[IndexBundleActivator] open data directories.."<<std::endl;
    bool bOpenDataDir = openDataDirectories_();
    SF1R_ENSURE_INIT(bOpenDataDir);
    std::cout<<"["<<config_->collectionName_<<"]"<<"[IndexBundleActivator] open id manager.."<<std::endl;
    idManager_ = createIDManager_();
    SF1R_ENSURE_INIT(idManager_);
    laManager_ = createLAManager_();
    SF1R_ENSURE_INIT(laManager_);
    SF1R_ENSURE_INIT(initializeQueryManager_());
    std::cout<<"["<<config_->collectionName_<<"]"<<"[IndexBundleActivator] open index manager.."<<std::endl;
    indexManager_ = createIndexManager_();
    SF1R_ENSURE_INIT(indexManager_);
    std::cout<<"["<<config_->collectionName_<<"]"<<"[IndexBundleActivator] open document manager.."<<std::endl;
    documentManager_ = createDocumentManager_();
    SF1R_ENSURE_INIT(documentManager_);
    documentManager_->setLangId(config_->languageIdentifierDbPath_);
    std::cout<<"["<<config_->collectionName_<<"]"<<"[IndexBundleActivator] open ranking manager.."<<std::endl;
    rankingManager_ = createRankingManager_();
    SF1R_ENSURE_INIT(rankingManager_);
    std::cout<<"["<<config_->collectionName_<<"]"<<"[IndexBundleActivator] open search manager.."<<std::endl;
    searchManager_ = createSearchManager_();
    SF1R_ENSURE_INIT(searchManager_);
    workerService_ = createWorkerService_();
    SF1R_ENSURE_INIT(workerService_);
    aggregatorManager_ = createAggregatorManager_();
    SF1R_ENSURE_INIT(aggregatorManager_);
    pQA_ = Singleton<ilplib::qa::QuestionAnalysis>::get();

    searchService_ = new IndexSearchService(config_);

    searchService_->aggregatorManager_ = aggregatorManager_;
    searchService_->workerService_ = workerService_;
    searchService_->workerService_->laManager_ = laManager_;
    searchService_->workerService_->idManager_ = idManager_;
    searchService_->workerService_->documentManager_ = documentManager_;
    searchService_->workerService_->indexManager_ = indexManager_;
    //searchService_->workerService_->rankingManager_ = rankingManager_;
    searchService_->workerService_->searchManager_ = searchManager_;
    searchService_->workerService_->pQA_ = pQA_;

    taskService_ = new IndexTaskService(config_, directoryRotator_, indexManager_);

    //taskService_->workerService_ = workerService_;
    //taskService_->workerService_->summarizer_.init(documentManager_->getLangId(), idManager_);
    taskService_->idManager_ = idManager_;
    taskService_->laManager_ = laManager_;
    taskService_->documentManager_ = documentManager_;
    taskService_->searchManager_ = searchManager_;
    taskService_->summarizer_.init(documentManager_->getLangId(), idManager_);



    return true;
}

std::string IndexBundleActivator::getCurrentCollectionDataPath_() const
{
    return config_->collPath_.getCollectionDataPath()+"/"+currentCollectionDataName_;
}

std::string IndexBundleActivator::getCollectionDataPath_() const
{
    return config_->collPath_.getCollectionDataPath();
}

std::string IndexBundleActivator::getQueryDataPath_() const
{
    return config_->collPath_.getQueryDataPath();
}

bool IndexBundleActivator::openDataDirectories_()
{
    std::vector<std::string>& directories = config_->collectionDataDirectories_;
    if( directories.size() == 0 )
    {
        std::cout<<"no data dir config"<<std::endl;
        return false;
    }
    directoryRotator_.setCapacity(directories.size());
    typedef std::vector<std::string>::const_iterator iterator;
    for (iterator it = directories.begin(); it != directories.end(); ++it)
    {
        bfs::path dataDir = bfs::path( getCollectionDataPath_() ) / *it;
        if (!directoryRotator_.appendDirectory(dataDir))
        {
            std::string msg = dataDir.file_string() + " corrupted, delete it!";
            DLOG(ERROR) <<msg <<endl;
            //clean the corrupt dir
            boost::filesystem::remove_all( dataDir );
            directoryRotator_.appendDirectory(dataDir);
        }
    }

    directoryRotator_.rotateToNewest();
    boost::shared_ptr<Directory> newest = directoryRotator_.currentDirectory();
    if (newest)
    {
        bfs::path p = newest->path();
        currentCollectionDataName_ = p.filename();
        //std::cout << "Current Index Directory: " << indexPath_() << std::endl;
        return true;
    }

    return false;
}

boost::shared_ptr<IDManager>
IndexBundleActivator::createIDManager_() const
{
    std::string dir = getCurrentCollectionDataPath_()+"/id/";
    boost::filesystem::create_directories(dir);

    boost::shared_ptr<IDManager> ret(
        new IDManager(dir)
    );

    return ret;
}

boost::shared_ptr<DocumentManager>
IndexBundleActivator::createDocumentManager_() const
{
    std::string dir = getCurrentCollectionDataPath_()+"/dm/";
    boost::filesystem::create_directories(dir);
    boost::shared_ptr<DocumentManager> ret(
        new DocumentManager(
            dir,
            config_->schema_,
            config_->encoding_,
            config_->documentCacheNum_
        )
    );

    return ret;
}

boost::shared_ptr<IndexManager>
IndexBundleActivator::createIndexManager_() const
{
    std::string dir = getCurrentCollectionDataPath_()+"/index/";
    boost::filesystem::create_directories(dir);
    boost::shared_ptr<IndexManager> ret;

    ret.reset(new IndexManager());
    if (ret)
    {
        IndexManagerConfig config(config_->indexConfig_);
        config.indexStrategy_.indexLocation_ = dir;


        IndexerCollectionMeta indexCollectionMeta;
        indexCollectionMeta.setName(config_->collectionName_);
        typedef IndexBundleSchema::iterator prop_iterator;
        for (prop_iterator iter = config_->schema_.begin(),
                                   iterEnd = config_->schema_.end();
               iter != iterEnd; ++iter)
        {
            IndexerPropertyConfig indexerPropertyConfig(
                iter->getPropertyId(),
                iter->getName(),
                iter->isIndex(),
                iter->isAnalyzed()
            );
            indexerPropertyConfig.setIsFilter(iter->getIsFilter());
            indexerPropertyConfig.setIsMultiValue(iter->getIsMultiValue());
            indexerPropertyConfig.setIsStoreDocLen(iter->getIsStoreDocLen());
            indexCollectionMeta.addPropertyConfig(indexerPropertyConfig);
        }

        config.addCollectionMeta(indexCollectionMeta);

        std::map<std::string, unsigned int> collectionIdMapping;
        collectionIdMapping[config_->collectionName_] = 1;

        ret->setIndexManagerConfig(config, collectionIdMapping);
    }

    return ret;
}

boost::shared_ptr<RankingManager>
IndexBundleActivator::createRankingManager_() const
{
    boost::shared_ptr<RankingManager> ret(new RankingManager);
    ret->init(config_->rankingManagerConfig_.rankingConfigUnit_);

    typedef std::map<std::string, float>::const_iterator weight_map_iterator;
    PropertyConfig propertyConfigOut;
    for (weight_map_iterator it = config_->rankingManagerConfig_.propertyWeightMapByProperty_.begin(),
                                        itEnd = config_->rankingManagerConfig_.propertyWeightMapByProperty_.end();
          it != itEnd; ++it)
    {
        if (config_->getPropertyConfig(it->first, propertyConfigOut))
        {
            ret->setPropertyWeight(propertyConfigOut.getPropertyId(), it->second);
        }
    }

    return ret;
}

boost::shared_ptr<SearchManager>
IndexBundleActivator::createSearchManager_() const
{
    boost::shared_ptr<SearchManager> ret;

    if (indexManager_ && idManager_ && documentManager_ && rankingManager_)
    {
        ret.reset(
            new SearchManager(
                config_->schema_,
                idManager_,
                documentManager_,
                indexManager_,
                rankingManager_,
                config_
            )
        );
    }
    return ret;
}

boost::shared_ptr<LAManager>
IndexBundleActivator::createLAManager_() const
{
    boost::shared_ptr<LAManager> ret(new LAManager());
    std::string kma_path;
    LAPool::getInstance()->get_kma_path(kma_path);
    string temp = kma_path + "/stopword.txt";
    ret->loadStopDict( temp );
    return ret;
}

boost::shared_ptr<WorkerService>
IndexBundleActivator::createWorkerService_()
{
    boost::shared_ptr<WorkerService> ret(new WorkerService(config_, directoryRotator_));
    return ret;
}

boost::shared_ptr<AggregatorManager>
IndexBundleActivator::createAggregatorManager_() const
{
    boost::shared_ptr<AggregatorManager> ret(new AggregatorManager());
    ret->setAggregatorConfig(config_->aggregatorConfig_);
    MasterNodeManagerSingleton::get()->registerAggregator(ret);
    ret->initLocalWorkerCaller(workerService_);
    ret->TOP_K_NUM = config_->topKNum_;
    ret->debug_ = true;
    return ret;
}

bool IndexBundleActivator::initializeQueryManager_() const
{
    // initialize Query Parser
    QueryParser::initOnlyOnce();

    std::string kma_path;
    LAPool::getInstance()->get_kma_path(kma_path);
    std::string restrictDictPath = kma_path + "/restrict.txt";
    QueryUtility::buildRestrictTermDictionary( restrictDictPath, idManager_);

    IndexBundleSchema::const_iterator docSchemaIter = config_->schema_.begin();
    for(; docSchemaIter != config_->schema_.end(); docSchemaIter++)
    {
        QueryManager::CollPropertyKey_T key( make_pair(config_->collectionName_, docSchemaIter->getName() ) );

        // Set Search Property
        if ( docSchemaIter->isIndex() )
            QueryManager::addCollectionSearchProperty(key);

        // Set Display Property
        QueryManager::addCollectionPropertyInfo(key, docSchemaIter->getType() );

        DisplayProperty displayProperty(docSchemaIter->getName());
        displayProperty.isSnippetOn_ = docSchemaIter->bSnippet_;
        displayProperty.isSummaryOn_ = docSchemaIter->bSummary_;
        displayProperty.summarySentenceNum_ = docSchemaIter->summaryNum_;
        displayProperty.isHighlightOn_ = docSchemaIter->bHighlight_;

        QueryManager::addCollectionDisplayProperty(key, displayProperty);
    }

    return true;
}


}
