#ifndef SF1R_B5MMANAGER_RAWSCDGENERATOR_H_
#define SF1R_B5MMANAGER_RAWSCDGENERATOR_H_

#include <string>
#include <vector>
#include "offer_db.h"

namespace sf1r {
    class RawScdGenerator {
    public:
        RawScdGenerator(OfferDb* odb, int mode);

        void LoadMobileSource(const std::string& file);

        void Process(Document& doc, int& type);

        bool Generate(const std::string& scd_file, const std::string& mdb_instance);

    private:

    private:
        OfferDb* odb_;
        int mode_;
        boost::unordered_set<std::string> mobile_source_;
    };

}

#endif

