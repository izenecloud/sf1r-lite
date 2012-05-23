#ifndef SF1R_B5MMANAGER_B5MOSCDGENERATOR_H_
#define SF1R_B5MMANAGER_B5MOSCDGENERATOR_H_

#include <string>
#include <vector>
#include "offer_db.h"

namespace sf1r {
    class B5moScdGenerator {
    public:
        B5moScdGenerator( OfferDb* odb);


        bool Generate(const std::string& mdb_instance);


    private:
        OfferDb* odb_;
    };

}

#endif

