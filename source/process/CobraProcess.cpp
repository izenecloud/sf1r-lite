#include "CobraProcess.h"
#include "RouterInitializer.h"

#include <common/SFLogger.h>
#include <la-manager/LAPool.h>
#include <license-manager/LicenseManager.h>
#include <query-manager/QMCommonFunc.h>

#include <bundles/querylog/QueryLogBundleConfiguration.h>
#include <bundles/querylog/QueryLogBundleActivator.h>

#include <OnSignal.h>
#include <common/XmlConfigParser.h>
#include <common/CollectionManager.h>
#include <controllers/CollectionHandler.h>
#include <controllers/Sf1Controller.h>

#include <util/ustring/UString.h>
#include <util/driver/IPRestrictor.h>
#include <util/driver/DriverConnectionFirewall.h>
#include <util/singleton.h>

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

namespace bfs = boost::filesystem;

#define ipRestrictor ::izenelib::driver::IPRestrictor::getInstance()

bool CobraProcess::initialize(const std::string& configFileDir)
{
    if( !exists(configFileDir) || !is_directory(configFileDir) ) return false;
    bfs::path logPath(bfs::path(".") / "log" / "COBRA");
    if( !sflog->init( logPath.string() ) ) return false;
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

    if(!initLAManager()) return false;

    if(!initFireWall()) return false;

    if(!initLicenseManager()) return false;

    initDriverServer();

    return true;
}

bool CobraProcess::initLAManager()
{
    LAManagerConfig laConfig;
    SF1Config::get()->getLAManagerConfig(laConfig);

    ///TODO 
    /// Ugly here, to be optimized through better configuration
    LAConfigUnit config;
    config.setId( "la_mia" );
    config.setAnalysis( "korean" );
    config.setMode( "label" );
    config.setDictionaryPath( laConfig.kma_path_ ); // defined macro
	
    laConfig.addLAConfig(config);
	
    AnalysisInfo analysisInfo;
    analysisInfo.analyzerId_ = "la_mia";
    analysisInfo.tokenizerNameList_.insert("tok_divide");
    laConfig.addAnalysisPair(analysisInfo);
	
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
    LAPool::getInstance()->set_kma_path(laConfig.kma_path_);
    //start dynamic update
    std::string kma_path;
    LAPool::getInstance()->get_kma_path(kma_path);
    
    std::string restrictDictPath = kma_path + "/restrict.txt";
    boost::shared_ptr<UpdatableRestrictDict> urd;
    unsigned int lastModifiedTime = static_cast<unsigned int>(
            la::getFileLastModifiedTime( restrictDictPath.c_str() ) );
    urd.reset( new UpdatableRestrictDict( lastModifiedTime ) );
    la::UpdateDictThread::staticUDT.addRelatedDict( restrictDictPath.c_str(), urd );
    la::UpdateDictThread::staticUDT.setCheckInterval(300);
    la::UpdateDictThread::staticUDT.start();

    atexit(&LAPool::destroy);

    return true;
}

QueryLogSearchService* CobraProcess::initQuery()
{
    ilplib::qa::QuestionAnalysis* pQA = Singleton<ilplib::qa::QuestionAnalysis>::get();
    std::string qahome = SF1Config::get()->getResourceDir();
    bfs::path path(bfs::path(qahome) / "qa" / "questionwords.txt");
    std::string qaPath = path.string();
    if( boost::filesystem::exists(qaPath) )
    {
        pQA->load(qaPath);
    }

    ///create QueryLogBundle
    boost::shared_ptr<QueryLogBundleConfiguration> queryLogBundleConfig
    (new QueryLogBundleConfiguration(SF1Config::get()->queryLogBundleConfig_));
    std::string bundleName = "QueryLogBundle";
    OSGILauncher& launcher = CollectionManager::get()->getOSGILauncher();
    launcher.start(queryLogBundleConfig);
    QueryLogSearchService* service = static_cast<QueryLogSearchService*>(launcher.getService(bundleName, "QueryLogSearchService"));

    //addExitHook(boost::bind(&OSGILauncher::stop, launcher));

    return service;
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
        sflog->error(SFL_INIT, "License Request File is failed to generated. Please check if you're a sudoer");
        return false;
    }

    if ( !licenseManager->validateLicenseFile() )
    {
        std::cerr << "[Warning] : license is invalid. Now sf1 will be worked on trial mode." << std::endl;
        sflog->error(SFL_INIT, "license is invalid. Now sf1 will be worked on trial mode.");
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
    QueryLogSearchService* service = initQuery();
    initializeDriverRouter(*router_, service, enableTest);

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

void CobraProcess::stopDriver()
{
    if (driverServer_)
    {
        driverServer_->stop();
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


#ifdef  EXIST_LICENSE
            char* home = getenv("HOME");
            std::string licenseDir = home; licenseDir += "/sf1-license/";

            {
                std::string filePath = licenseDir + LicenseManager::TOKEN_FILENAME;
                std::string token("");
                if ( !LicenseManager::extract_token_from(filePath, token) )
                {
                    return false;
                }

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
#endif

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

