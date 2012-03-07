#ifndef SF1R_B5MMANAGER_LOGSERVERHANDLER_H_
#define SF1R_B5MMANAGER_LOGSERVERHANDLER_H_

#include <string>
#include <vector>
#include <boost/unordered_map.hpp>
#include <boost/regex.hpp>
#include <configuration-manager/LogServerConnectionConfig.h>
#include <log-manager/LogServerConnection.h>

namespace sf1r {
    class LogServerHandler {
    public:
        LogServerHandler();

        void SetLogServerConfig(const LogServerConnectionConfig& config)
        {
            logserver_config_ = config;
        }

        bool Send(const std::string& scd_path, const std::string& work_dir = "");
        bool QuickSend(const std::string& scd_path);

    private:
        void Post_(LogServerConnection& conn, const std::string& suuid, const std::vector<std::string>& sdocname_list);
    private:
        LogServerConnectionConfig logserver_config_;
    };

}

#endif

