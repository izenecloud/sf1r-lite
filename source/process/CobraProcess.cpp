#include "CobraProcess.h"
#include "RouterInitializer.h"

#include <common/SFLogger.h>
#include <la-manager/LAPool.h>
#include <license-manager/LicenseManager.h>

#include <OnSignal.h>
#include <common/XmlConfigParser.h>
#include <common/CollectionManager.h>
#include <controllers/Sf1Controller.h>

#include <util/ustring/UString.h>
#include <util/driver/IPRestrictor.h>
#include <util/driver/DriverConnectionFirewall.h>

#include <boost/thread.hpp>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/path.hpp>

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

    if(!initFireWall()) return false;

    if(!initLicenseManager()) return false;

    initDriverServer();

    return true;
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

//    We do not need to monitor Index Size for license now.
//    // ------------------------------ [ License Manager ]
//    // Extract collectionDataPathList from collectionMeta in baConfig
//    std::vector<std::string> collDataPathList;
//    const std::map<std::string, CollectionMeta>&
//    collectionMetaMap = SF1Config::get()->getCollectionMetaMap();
//    std::map<std::string, CollectionMeta>::const_iterator
//        collectionIter = collectionMetaMap.begin();
//    for(; collectionIter != collectionMetaMap.end(); collectionIter++)
//    {
//        collDataPathList.push_back( collectionIter->second.getCollectionPath().getCollectionDataPath() );
//        collDataPathList.push_back( collectionIter->second.getCollectionPath().getQueryDataPath() );
//    }
//    boost::thread bgThread(boost::bind(&LicenseManager::startBGWork,*licenseManager, collDataPathList));

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
    initializeDriverRouter(*router_, enableTest);

    boost::shared_ptr<DriverConnectionFactory> factory(
        new DriverConnectionFactory(router_)
    );
    factory->setFirewall(DriverConnectionFirewall());

    driverServer_.reset(
        new DriverServer(endpoint, factory, threadPoolSize)
    );
    return true;
}

int CobraProcess::run()
{
    atexit(&LAPool::destroy);

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


    
/*    
        // initialize BA
        boost::shared_ptr<BAMain> ba(new BAMain);
        if (!ba->init(config_.getBAParams()))
        {
            std::cerr << "BA Initialization Failed. "
                      "Please check error log file."
                      << std::endl;
            return 1;
        }
        addExitHook(boost::bind(&BAMain::stop, ba));

        std::size_t collectionCount = config_.collections.size();
        std::vector<boost::shared_ptr<SMIAProcess> > smiaList(collectionCount);

        std::set<std::string>::const_iterator collectionIt
        = config_.collections.begin();
        for (std::size_t i = 0; i < collectionCount; ++i, ++collectionIt)
        {
            const std::string& collection = *collectionIt;

            smiaList[i].reset(new SMIAProcess);
            if (!smiaList[i]->init(config_.getSMIAParams(collection)))
            {
                std::cerr << "SMIA (" << collection
                          << ") Initialization Failed. "
                          << "Please check error log file."
                          << std::endl;
                return 1;
            }
            addExitHook(boost::bind(&SMIAProcess::stop, smiaList[i]));
        }

        //do some static work
        if ( collectionCount>0 )
        {
            QuerySupportConfig query_support_config = SF1Config::get()->getQuerySupportConfig();

            MiningQueryLogHandler* handler = MiningQueryLogHandler::getInstance();
            handler->SetParam(query_support_config.update_time, query_support_config.log_days);
            std::string recommend_cron_string = SF1Config::get()->getIndexRecommendCronString();
            if ( !handler->cronStart(recommend_cron_string) )
            {
                std::cout<<"Can not start cron job for recommend, cron_string: "<<recommend_cron_string<<std::endl;
            }
        }
*/
        setupDefaultSignalHandlers();

        // Starts processes in threads
        boost::thread_group threads;
/*		
        threads.create_thread(boost::bind(&BAMain::run, ba));
        for (std::size_t i = 0; i < collectionCount; ++i)
        {
            threads.create_thread(boost::bind(&SMIAProcess::run, smiaList[i]));
        }
*/
        threads.join_all();
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

