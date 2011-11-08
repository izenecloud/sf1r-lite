#include "CobraProcess.h"
#include "RouterInitializer.h"

#include <common/SFLogger.h>
#include <la-manager/LAPool.h>
#include <license-manager/LicenseManager.h>
#include <aggregator-manager/MasterServer.h>
#include <node-manager/NodeManager.h>
#include <node-manager/MasterNodeManager.h>
#include <mining-manager/query-correction-submanager/QueryCorrectionSubmanager.h>

#include <OnSignal.h>
#include <common/XmlConfigParser.h>
#include <common/CollectionManager.h>
#include <controllers/CollectionHandler.h>
#include <controllers/Sf1Controller.h>

#include <util/ustring/UString.h>
#include <util/driver/IPRestrictor.h>
#include <util/driver/DriverConnectionFirewall.h>
#include <util/singleton.h>
#include <3rdparty/zookeeper/ZooKeeper.hpp>

#include <question-answering/QuestionAnalysis.h>

#include <boost/thread.hpp>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/shared_ptr.hpp>

#include <iostream>
#include <sstream>
#include <string>
#include <set>
#include <vector>
#include <map>

using namespace sf1r;
using namespace boost::filesystem;
using namespace izenelib::util;
using namespace izenelib::driver;
using namespace zookeeper;

namespace bfs = boost::filesystem;

#define ipRestrictor ::izenelib::driver::IPRestrictor::getInstance()

bool CobraProcess::initialize(const std::string& configFileDir)
{
    if( !exists(configFileDir) || !is_directory(configFileDir) ) return false;
    try
    {
        configDir_ = configFileDir;
        boost::filesystem::path p(configFileDir);
        SF1Config::get()->setHomeDirectory(p.string());
        if( !SF1Config::get()->parseConfigFile( bfs::path(p/"sf1config.xml").string() ) )
        {
            return false;
        }
    }
    catch ( izenelib::util::ticpp::Exception & e )
    {
        cerr << e.what() << endl;
        return false;
    }

    if(!initLogManager()) return false;

    if(!initFireWall()) return false;

    if(!initLicenseManager()) return false;

    initDriverServer();

    initNodeManager();

    return true;
}

bool CobraProcess::initLogManager()
{
    std::string log_conn = SF1Config::get()->getLogConnString();
    std::string cassandra_conn = SF1Config::get()->getCassandraConnString();
    if (!sflog->init(log_conn))
    {
        std::cerr << "Init LogManager with " << log_conn << " failed!" << std::endl;
        return false;
    }
    if (!sflog->initCassandra(cassandra_conn))
    {
        std::cerr << "Init CassandraConnection with " << cassandra_conn << " failed!" << std::endl;
        return false;
    }
    return true;
}

bool CobraProcess::initLAManager()
{
    LAManagerConfig laConfig;
    SF1Config::get()->getLAManagerConfig(laConfig);

    ///TODO
    /// Ugly here, to be optimized through better configuration
    LAConfigUnit config2;
    config2.setId( "la_sia" );
    config2.setAnalysis( "korean" );
    config2.setMode( "label" );
    config2.setDictionaryPath( laConfig.kma_path_ ); // defined macro

    laConfig.addLAConfig(config2);

    AnalysisInfo analysisInfo2;
    analysisInfo2.analyzerId_ = "la_sia";
    analysisInfo2.tokenizerNameList_.insert("tok_divide");
    laConfig.addAnalysisPair(analysisInfo2);

    if (! LAPool::getInstance()->init(laConfig))
        return false;

    return true;
}

void CobraProcess::initQuery()
{
    ilplib::qa::QuestionAnalysis* pQA = Singleton<ilplib::qa::QuestionAnalysis>::get();
    const std::string& qahome = SF1Config::get()->getResourceDir();
    bfs::path path(bfs::path(qahome) / "qa" / "questionwords.txt");
    std::string qaPath = path.string();
    if( boost::filesystem::exists(qaPath) )
    {
        pQA->load(qaPath);
    }
    QueryCorrectionSubmanager::system_resource_path_ = SF1Config::get()->getResourceDir();
    QueryCorrectionSubmanager::system_working_path_ = SF1Config::get()->getWorkingDir();
    QueryCorrectionSubmanager::getInstance();
}

bool CobraProcess::initLicenseManager()
{
#ifdef COBRA_RESTRICT
    boost::shared_ptr<LicenseManager> licenseManager;
    // license manager initialization.
    char* home = getenv("HOME");
    std::string licenseDir = home; licenseDir += "/sf1-license/";
    try { // If license directory is not exist, create it.
        if ( !boost::filesystem::exists(licenseDir) ) {
            std::cout << "[Warning] : " << licenseDir << " is Created." << std::endl;
            boost::filesystem::create_directories(licenseDir);
        }
    } catch (boost::filesystem::filesystem_error& e) {
        std::cerr << "Error : " << e.what() << std::endl;
        return false;
    }
    std::string path = licenseDir + LicenseManager::LICENSE_KEY_FILENAME;
    licenseManager.reset( new LicenseManager("1.0.0", path, false) );

    path = licenseDir + LicenseManager::LICENSE_REQUEST_FILENAME;
    if ( !licenseManager->createLicenseRequestFile(path) )
    {
        DLOG(ERROR) <<"License Request File is failed to generated. Please check if you're a sudoer" <<endl;
        return false;
    }

    if ( !licenseManager->validateLicenseFile() )
    {
        std::cerr << "[Warning] : license is invalid. Now sf1 will be worked on trial mode." << std::endl;
        LicenseManager::continueIndex_ = false;
    }

#endif // COBRA_RESTRICT
    return true;
}

bool CobraProcess::initFireWall()
{
    const FirewallConfig& fwConfig = SF1Config::get()->getFirewallConfig();
    std::vector<std::string>::const_iterator iter;
    for(iter = fwConfig.allowIPList_.begin(); iter != fwConfig.allowIPList_.end(); iter++)
        if ( !ipRestrictor->registerAllowIP( *iter ) )
            return false;
    for(iter = fwConfig.denyIPList_.begin(); iter != fwConfig.denyIPList_.end(); iter++)
        if ( !ipRestrictor->registerDenyIP( *iter ) )
            return false;
    return true;
}

bool CobraProcess::initDriverServer()
{
    const BrokerAgentConfig& baConfig = SF1Config::get()->getBrokerAgentConfig();
    std::size_t threadPoolSize = baConfig.threadNum_;
    bool enableTest = baConfig.enableTest_;
    unsigned int port = baConfig.port_;

    boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::tcp::v4(),port);

    // init Router
    router_.reset(new ::izenelib::driver::Router);
    initQuery();
    initializeDriverRouter(*router_, NULL, enableTest);

    boost::shared_ptr<DriverConnectionFactory> factory(
        new DriverConnectionFactory(router_)
    );
    factory->setFirewall(DriverConnectionFirewall());

    driverServer_.reset(
        new DriverServer(endpoint, factory, threadPoolSize)
    );

    addExitHook(boost::bind(&CobraProcess::stopDriver, this));

    return true;
}

bool CobraProcess::initNodeManager()
{
    NodeManagerSingleton::get()->initWithConfig(
            SF1Config::get()->distributedTopologyConfig_,
            SF1Config::get()->distributedUtilConfig_
            );

    return true;
}

void CobraProcess::stopDriver()
{
    if (driverServer_)
    {
        driverServer_->stop();
    }
}

bool CobraProcess::startDistributedServer()
{
    if (!SF1Config::get()->distributedTopologyConfig_.enabled_)
        return false;

    // Register current SF1 node through zookeeper
    NodeManagerSingleton::get()->registerNode();

    if (sf1r::SF1Config::get()->isWorkerEnabled())
    {
        try
        {
            // worker rpc server
            std::string localHost = SF1Config::get()->distributedTopologyConfig_.curSF1Node_.host_;
            uint16_t workerPort = SF1Config::get()->distributedTopologyConfig_.curSF1Node_.workerAgent_.port_;
            std::size_t threadNum = SF1Config::get()->brokerAgentConfig_.threadNum_;
            workerServer_.reset(new WorkerServer(localHost, workerPort, threadNum));
            workerServer_->start();
            cout << "#[Worker Server]started, listening at localhost:"<<workerPort<<" ..."<<endl;

            // master notifier, xxx
            //std::string masterHost = SF1Config::get()->distributedTopologyConfig_.curSF1Node_.workerAgent_.masterHost_;
            //uint16_t masterPort = SF1Config::get()->distributedTopologyConfig_.curSF1Node_.workerAgent_.masterPort_;
            //MasterNotifierSingleton::get()->setMasterServerInfo(masterHost, masterPort);

            // Let current node work as Worker
            NodeManagerSingleton::get()->registerWorker();
        }
        catch (std::exception& e)
        {
            cout << e.what() << endl;
        }
    }

    if (SF1Config::get()->isMasterEnabled())
    {
        // master rpc server
        //MasterServer::get()->start(curNodeInfo.localHost_, masterPort);

        // Let current node work as Master
        NodeManagerSingleton::get()->registerMaster();

        // Initialize & Start Master Node Manager
        MasterNodeManagerSingleton::get()->init();
        MasterNodeManagerSingleton::get()->startServer();
    }

    addExitHook(boost::bind(&CobraProcess::stopDistributedServer, this));

    return true;
}

void CobraProcess::stopDistributedServer()
{
    NodeManagerSingleton::get()->deregisterNode();

    if (workerServer_)
    {
        workerServer_->end();
        workerServer_->join();
    }
}

int CobraProcess::run()
{
    setupDefaultSignalHandlers();

    bool caughtException = false;

    try
    {
        bfs::directory_iterator iter(configDir_), end_iter;
        for(; iter!= end_iter; ++iter)
        {
            if(bfs::is_regular_file(*iter))
            {
                if(iter->filename().rfind(".xml") == (iter->filename().length() - std::string(".xml").length()))
                    if(iter->filename() != "sf1config.xml")
                    {
                        std::string collectionName = iter->filename().substr(0,iter->filename().rfind(".xml"));
                        CollectionManager::get()->startCollection(collectionName, iter->string());
                    }
            }
        }

        // in collection config file, each <Indexing analyzer="..."> needs to be initialized in LAPool,
        // so LAPool is initialized here after all collection config files are parsed
        if(!initLAManager())
            throw std::runtime_error("failed in initLAManager()");

#ifdef  EXIST_LICENSE
            char* home = getenv("HOME");
            std::string licenseDir = home; licenseDir += "/sf1-license/";

            {
                std::string filePath = licenseDir + LicenseManager::TOKEN_FILENAME;
                if( boost::filesystem::exists(filePath) )
                {
                    std::string token("");
                    LicenseManager::extract_token_from(filePath, token);

                    ///Insert the extracted token into the deny control lists for all collections
                    std::map<std::string, CollectionMeta>&
                        collectionMetaMap = SF1Config::get()->mutableCollectionMetaMap();
                    std::map<std::string, CollectionMeta>::iterator
                        collectionIter = collectionMetaMap.begin();

                    for(; collectionIter != collectionMetaMap.end(); collectionIter++)
                    {
                        CollectionMeta& collectionMeta = collectionIter->second;
                        collectionMeta.aclDeny(token);
                    }
                }
            }
#endif

        startDistributedServer();

        driverServer_->run();
    }
    catch (const std::exception& e)
    {
        caughtException = true;
        std::cout << "Caught std exception:  "
                  << e.what() << std::endl;
    }
    catch (...)
    {
        caughtException = true;
        cout<<"Exit, catch exception here"<<endl;
    }

    return caughtException ? 1 : 0;
}
