#ifndef SF1R_B5MMANAGER_UUEGENERATOR_H_
#define SF1R_B5MMANAGER_UUEGENERATOR_H_

#include <string>
#include <vector>
#include "b5m_types.h"
#include "b5m_helper.h"
#include "offer_db.h"
#include <configuration-manager/LogServerConnectionConfig.h>
#include <log-manager/LogServerConnection.h>

namespace sf1r {

    class UueGenerator {
    public:
        UueGenerator(const LogServerConnectionConfig& config, OfferDb* odb);
        UueGenerator(OfferDb* odb);

        bool Generator(const std::string& b5mo_scd, const std::string& result);

    private:
        LogServerConnectionConfig logserver_config_;
        OfferDb* odb_;
    };

}

#endif

