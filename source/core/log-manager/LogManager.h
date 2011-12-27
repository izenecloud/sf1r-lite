/// @details
/// - Log
///     - 2009.12.21 Replace pthread part to boost::thread and remove cout message by Dohyun Yun
#ifndef LOGMANAGER_H_
#define LOGMANAGER_H_

#include <string>
#include <time.h>
#include <ctime>
#include <cstdlib>
#include <cstddef>
#include <cstdarg>
#include <sys/time.h>

#include "LogManagerSingleton.h"

namespace sf1r
{

class LogManager : public LogManagerSingleton<LogManager>
{

public:

    typedef LogManager* LogManagerPtr;

    /// constructor
    LogManager();

    /// destructor
    ~LogManager();

    /**
     * @brief Initializes the logger
     *
     * @param path      The prefix of the path. E.g. ./log/sf1.collection
     * @param language
     * @param nIsInfoOn
     * @param nIsVerbOn
     * @param nIsDebugOn
     **/
    bool init(const std::string& logPath, const std::string& language = "english");

    bool initCassandra(const std::string& logPath);

    /// delete whole database file
    bool del_database();
};

} //end - namespace sf1r

#endif /*LOGMANAGER_H_*/
