#ifndef SF1R_B5MMANAGER_RAWSCDGENERATOR_H_
#define SF1R_B5MMANAGER_RAWSCDGENERATOR_H_

#include <string>
#include <vector>
#include "offer_db.h"
//#include "history_db_helper.h"

namespace sf1r {
    class RpcServerConnectionConfig;
    class RawScdGenerator {
    public:
        RawScdGenerator(OfferDb* odb,
            int mode,
            RpcServerConnectionConfig* img_server_config);

        void LoadMobileSource(const std::string& file);

        void Process(Document& doc, int& type);

        bool Generate(const std::string& scd_file, const std::string& mdb_instance);

    private:

    private:
        OfferDb* odb_;
        int mode_;
        boost::unordered_set<std::string> mobile_source_;
        RpcServerConnectionConfig* img_server_cfg_;
    };

}

#endif

