#ifndef SF1R_B5MMANAGER_LOGSERVERCLIENT_H_
#define SF1R_B5MMANAGER_LOGSERVERCLIENT_H_

#include <string>
#include <vector>
#include <boost/unordered_map.hpp>
#include <boost/regex.hpp>
#include <configuration-manager/LogServerConnectionConfig.h>
#include <log-manager/LogServerConnection.h>
#include "b5m_types.h"

namespace sf1r {
    class LogServerClient {
    public:


        static bool Init(const LogServerConnectionConfig& config);
        static bool Test();
        static void Update(const std::string& pid, const std::vector<std::string>& docid_list);
        static void AppendUpdate(const std::string& pid, const std::vector<std::string>& docid_list);
        static void RemoveUpdate(const std::string& pid, const std::vector<std::string>& docid_list);
        static bool GetPid(const std::string& docid, std::string& pid);
        static bool GetDocidList(const std::string& pid, std::vector<std::string>& docid_list);
        static void Flush();


    private:
    };

}

#endif

