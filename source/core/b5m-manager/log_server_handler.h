#ifndef SF1R_B5MMANAGER_LOGSERVERHANDLER_H_
#define SF1R_B5MMANAGER_LOGSERVERHANDLER_H_

#include <string>
#include <vector>
#include <boost/unordered_map.hpp>
#include <boost/unordered_set.hpp>
#include <boost/regex.hpp>
#include <configuration-manager/LogServerConnectionConfig.h>
#include <log-manager/LogServerConnection.h>
#include "b5m_types.h"
#include "offer_db.h"

namespace sf1r {
    class LogServerHandler {
    public:
        LogServerHandler(const LogServerConnectionConfig& config);


        bool Generate(const std::string& mdb_instance);

    private:
        LogServerConnectionConfig logserver_config_;
        //OfferDb* odb_;
        //std::string work_dir_;
    };

}

#endif

