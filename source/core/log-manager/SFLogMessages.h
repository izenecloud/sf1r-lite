#ifndef __SF_LOG_MESSAGES_H
#define __SF_LOG_MESSAGES_H

#include <iostream>
#include <string>
#include <map>
#include<util/ustring/UString.h>


/**
 * @name SfLogMessages.h
 * @date 2009-12-31
 * @author deepesh
 * @brief Log numbers for log messages in SF1
 * @details
 *  - Log
 *      - 2010.03.30 Dohyun Yun, Modified return type of getLogMsg()
 *
 */

#define TOT_ERRORS 200
namespace SFLogMessage
{
    static std::map<int, std::string> errMsgIdPair;
    //static bool bErrMsgInit = false;

    enum ERROR_NUM {
        NO_ERR=0,
        UNKNOWN=999999
    };


    /**
     * @brief get the error string correspondingto ID
     */
    std::string getLogMsg(int id);
    bool initLogMsg(const std::string& language);
    char* get_env_(const char* envVarName);

    void parse_Lang(const std::string& evnStr);
}

#endif// SF_LOG_MESSAGES
