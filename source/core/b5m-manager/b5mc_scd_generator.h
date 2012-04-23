#ifndef SF1R_B5MMANAGER_B5MCSCDGENERATOR_H_
#define SF1R_B5MMANAGER_B5MCSCDGENERATOR_H_

#include <string>
#include <vector>
#include <boost/unordered_map.hpp>
#include <boost/regex.hpp>
#include "offer_db.h"

namespace sf1r {
    class B5MCScdGenerator {
        typedef boost::unordered_map<std::string, std::string> ModifyMap;
    public:
        B5MCScdGenerator(const std::string& new_scd, const std::string& old_scd, OfferDb* odb, const std::string& output_dir);

        void Process(const UueItem& item);

        void Finish();

        //B5MCScdGenerator(OfferDb* odb, const std::string& mdb);

        //bool Generate(const std::string& scd_file, const std::string& output_dir);

    private:

    private:
        std::string new_scd_;
        std::string old_scd_;
        OfferDb* odb_;
        std::string output_dir_;
        ModifyMap modified_;
        //std::string work_path_;
    };

}

#endif

