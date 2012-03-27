#ifndef SF1R_B5MMANAGER_B5MCSCDGENERATOR_H_
#define SF1R_B5MMANAGER_B5MCSCDGENERATOR_H_

#include <string>
#include <vector>
#include <boost/unordered_map.hpp>
#include <boost/regex.hpp>
#include "offer_db.h"

namespace sf1r {
    class B5MCScdGenerator {
    public:
        B5MCScdGenerator(OfferDb* odb);


        bool Generate(const std::string& scd_file, const std::string& output_dir);

    private:

    private:
        OfferDb* odb_;
    };

}

#endif

