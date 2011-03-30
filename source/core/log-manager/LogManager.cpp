#include "LogManager.h"
#include "SFLogMessages.h"
#include "DbConnection.h"
#include "SystemEvent.h"
#include "UserQuery.h"

#include <boost/shared_ptr.hpp>
#include <boost/filesystem.hpp>
#include <boost/regex.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/bind.hpp>
#include <boost/any.hpp>

#include <util/ustring/algo.hpp>

#include <iostream>
#include <fstream>
#include <list>
#include <vector>
#include <map>
#include <string>
#include <stdio.h>
#include <signal.h>

using namespace std;
using namespace boost::filesystem;
using namespace boost::posix_time;

namespace sf1r
{

    const char SS_STATUS_NAME[][SFL_STAT_EOS] =
    {
        "SYS", "SCD", "LA", "IDX", "MINE", "INIT", "SRCH"
    };
    const char* LOG_TYPE_NAME[LOG_EOS] = { "info", "warn", "err", "crit"};

    LogManager::LogManager(){}

    bool LogManager::init(const std::string& pathParam, const std::string& language)
    {
        logPath = pathParam;
        path logPath(pathParam);
        path logDir = logPath.parent_path();
        if(exists(logDir)) {
            if (!is_directory(logDir)) {
                cerr << "Log Directory " << logDir << " is a file" << endl;
                return false;
            }
        } else {
            create_directories(logDir);
        }

        SFLogMessage::initLogMsg(language);

        if( !DbConnection::instance().init(pathParam) )
            return false;

        SystemEvent::createTable();
        UserQuery::createTable();
        return true;
    }

    LogManager::~LogManager()
    {
    }

    bool LogManager::del_database()
    {
         string order = "rm " + logPath;

         DbConnection::instance().close();
         std::system(order.c_str());
         if(!init(logPath))
             return false;

         return true;
    }

    void LogManager::writeEventLog( LOG_TYPE type, int nStatus, int nErrCode, va_list & vlist)
    {
        writeEventLog(type, nStatus, SFLogMessage::getLogMsg(nErrCode).c_str(), vlist);
    }

    void LogManager::writeEventLog( LOG_TYPE type, int nStatus, const char* msg, va_list & vlist)
    {
#define LOGGER_LOG_LEN 1024
        char logMsg[LOGGER_LOG_LEN+1];
        memset( logMsg, 0, LOGGER_LOG_LEN );
#ifdef WIN32
        _vsnprintf(logMsg, LOGGER_LOG_LEN, msg, vlist);
#else
        vsnprintf(logMsg, LOGGER_LOG_LEN, msg, vlist);
#endif
        logMsg[LOGGER_LOG_LEN] = 0;

        va_end(vlist);

        SystemEvent event;
        event.setLevel(LOG_TYPE_NAME[type]);
        event.setSource(SS_STATUS_NAME[nStatus]);
        event.setContent(logMsg);
        event.setTimeStamp(second_clock::local_time());
        event.save();
    }

//        execSql("create table user_click(query TEXT, collection TEXT, title TEXT, docid TEXT, session_id integer, timestamp TEXT);");

}//end -namespace sf1r
