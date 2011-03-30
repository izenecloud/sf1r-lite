/// @details
/// - Log
///     - 2009.12.21 Replace pthread part to boost::thread and remove cout message by Dohyun Yun
#ifndef LOGMANAGER_H_
#define LOGMANAGER_H_

#include <string>
#include <time.h>
#include <fstream>
#include <iostream>

#include "LogManagerSingleton.h"
#include "SFLoggerCode.h"

#include <am/sequence_file/SimpleSequenceFile.hpp>
#include <am/3rdparty/rde_hash.h>
#include <am/hdb_trie/hdb_trie.hpp>
#include <cache/IzeneCache.h>
#include <util/ThreadModel.h>

#include <boost/filesystem.hpp>
#include <boost/thread.hpp>
#include <boost/any.hpp>

namespace sf1r {

    class LogManager : public LogManagerSingleton<LogManager> {

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

            /// delete whole database file
            bool del_database();

            #define LOG_info  LOG_INFO
            #define LOG_error LOG_ERR
            #define LOG_warn  LOG_WARN
            #define LOG_crit  LOG_CRIT

            #define DECLARE_INTERFACE(NAME) \
            void NAME (int nStatus, int nErrCode, ...) \
            { \
                va_list arglist; \
                va_start( arglist, nErrCode ); \
                writeEventLog( LOG_ ## NAME, nStatus, nErrCode, arglist ); \
                va_end(arglist); \
            }\
            \
            void NAME (int nStatus, const char* msg, ...) \
            { \
                va_list arglist; \
                va_start( arglist, msg ); \
                writeEventLog( LOG_ ## NAME, nStatus, msg, arglist ); \
                va_end(arglist); \
            }\

            /**
             * void info (int nStatus, int nErrCode, ...);
             * void info (int nStatus, const char* msg, ...);
             */
            DECLARE_INTERFACE(info)

            /**
             * void error (int nStatus, int nErrCode, ...);
             * void error (int nStatus, const char* msg, ...);
             */
            DECLARE_INTERFACE(error)

            /**
             * void warn (int nStatus, int nErrCode, ...);
             * void warn (int nStatus, const char* msg, ...);
             */
            DECLARE_INTERFACE(warn)

            /**
             * void crit (int nStatus, int nErrCode, ...);
             * void crit (int nStatus, const char* msg, ...);
             */
            DECLARE_INTERFACE(crit)

        private:

            void writeEventLog( LOG_TYPE type, int nStatus, int nErrCode, va_list & vlist );

            void writeEventLog( LOG_TYPE type, int nStatus, const char* msg, va_list & vlist);

        private:
            string logPath;

    };

} //end - namespace sf1r

#endif /*LOGMANAGER_H_*/
