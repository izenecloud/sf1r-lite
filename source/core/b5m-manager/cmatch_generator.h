#ifndef SF1R_B5MMANAGER_CMATCHGENERATOR_H_
#define SF1R_B5MMANAGER_CMATCHGENERATOR_H_

#include <string>
#include <vector>
#include "offer_db.h"
#include "history_db_helper.h"

namespace sf1r {
    class CMatchGenerator {
    public:
        CMatchGenerator( OfferDb* odb);


        bool Generate(const std::string& mdb_instance);


    private:
        OfferDb* odb_;
    };

}

#endif

