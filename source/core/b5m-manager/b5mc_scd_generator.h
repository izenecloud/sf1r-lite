#ifndef SF1R_B5MMANAGER_B5MCSCDGENERATOR_H_
#define SF1R_B5MMANAGER_B5MCSCDGENERATOR_H_

#include <string>
#include <vector>
#include <boost/unordered_map.hpp>
#include <boost/regex.hpp>
#include "offer_db.h"

namespace sf1r {
    class B5mcScdGenerator {
    public:
        B5mcScdGenerator(OfferDb* odb);

        bool Generate(const std::string& scd_path, const std::string& mdb_instance);


    private:

        void Process_(Document& doc, int& type);

    private:
        OfferDb* odb_;
    };

}

#endif

