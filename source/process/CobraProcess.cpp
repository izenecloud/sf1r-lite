#include "CobraProcess.h"
#include <common/SFLogger.h>
#include <la-manager/LAPool.h>

#include <OnSignal.h>
#include <XmlConfigParser.h>

#include <util/ustring/UString.h>
#include <util/thread-pool/ThreadObjectPool.h>
#include <util/profiler/ProfilerGroup.h>

#include <boost/shared_ptr.hpp>
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

namespace bfs = boost::filesystem;

bool CobraProcess::initialize(const std::string& configFileDir)
{
    if( !exists(configFileDir) || !is_directory(configFileDir) ) return false;
    bfs::path logPath(bfs::path(".") / "log" / "COBRA");
    if( !sflog->init( logPath.string() ) ) return false;
    try
    {
        boost::filesystem::path p(configFileDir); 
        SF1Config::get()->setHomeDirectory(p.string());
        if( !SF1Config::get()->parseConfigFile( bfs::path(p/"sf1config.xml").string() ) )
        {
            return false;
        }
        bfs::directory_iterator iter(configFileDir), end_iter;
        for(; iter!= end_iter; ++iter)
        {
            if(bfs::is_regular_file(*iter))
            {
                if(iter->filename().rfind(".xml") == (iter->filename().length() - std::string(".xml").length()))
                    if(iter->filename() != "sf1config.xml")
                    {
                        std::string collectionName = iter->filename().substr(0,iter->filename().rfind(".xml"));
                        if(CollectionConfig::get()->parseConfigFile(collectionName, iter->string()))
                        {
                            //TODO
                        }
                    }
             }
        }		

    }
    catch ( izenelib::util::ticpp::Exception & e )
    {
        cerr << e.what() << endl;
        return false;
    }
    SF1Config::get()->getCobraConfig(config_);
    config_.setConfigPath( configFileDir );

    return true;
} // end - in


int CobraProcess::run()
{
    atexit(&LAPool::destroy);

    bool caughtException = false;

    try
    {
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

