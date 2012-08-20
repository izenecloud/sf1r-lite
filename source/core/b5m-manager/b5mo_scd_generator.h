#ifndef SF1R_B5MMANAGER_B5MOSCDGENERATOR_H_
#define SF1R_B5MMANAGER_B5MOSCDGENERATOR_H_

#include <string>
#include <vector>
#include "offer_db.h"
#include "history_db_helper.h"

namespace sf1r {
    class LogServerConnectionConfig;
    class B5moScdGenerator {
    public:
        B5moScdGenerator( OfferDb* odb, B5MHistoryDBHelper* hdb,
            LogServerConnectionConfig* config);


        bool Generate(const std::string& mdb_instance, const std::string& last_mdb_instance);


    private:
        OfferDb* odb_;
        B5MHistoryDBHelper* historydb_;
        LogServerConnectionConfig* log_server_cfg_;
    };

}

#endif

