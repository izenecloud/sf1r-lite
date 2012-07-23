#ifndef SF1R_B5MMANAGER_RAWSCDGENERATOR_H_
#define SF1R_B5MMANAGER_RAWSCDGENERATOR_H_

#include <string>
#include <vector>
#include "offer_db.h"
#include "history_db_helper.h"

namespace sf1r {
    class LogServerConnectionConfig;
    class RawScdGenerator {
    public:
        RawScdGenerator(OfferDb* odb, B5MHistoryDBHelper* hdb, int mode, LogServerConnectionConfig* config);

        void LoadMobileSource(const std::string& file);

        void Process(Document& doc, int& type);

        bool Generate(const std::string& scd_file, const std::string& mdb_instance);

    private:

    private:
        OfferDb* odb_;
        B5MHistoryDBHelper* historydb_;
        int mode_;
        LogServerConnectionConfig* log_server_cfg_;
        boost::unordered_set<std::string> mobile_source_;
    };

}

#endif

