#ifndef SF1R_B5MMANAGER_LOGSERVERHANDLER_H_
#define SF1R_B5MMANAGER_LOGSERVERHANDLER_H_

#include <string>
#include <vector>
#include <boost/unordered_map.hpp>
#include <boost/regex.hpp>
#include <configuration-manager/LogServerConnectionConfig.h>
#include <log-manager/LogServerConnection.h>
#include "b5m_types.h"

namespace sf1r {
    class LogServerHandler {
    public:
        LogServerHandler(const LogServerConnectionConfig& config, bool reindex = false);

        bool Open();

        void Process(const BuueItem& item);

        void Finish();

    private:
        LogServerConnectionConfig logserver_config_;
        bool reindex_;
        bool open_;
    };

}

#endif

