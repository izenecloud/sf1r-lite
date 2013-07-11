#ifndef SF1R_B5MMANAGER_B5MCSCDGENERATOR_H_
#define SF1R_B5MMANAGER_B5MCSCDGENERATOR_H_

#include <string>
#include <vector>
//#include "brand_db.h"
#include "product_matcher.h"

namespace sf1r {
    class B5mcScdGenerator {
    public:
        B5mcScdGenerator(int mode, ProductMatcher* matcher = NULL);

        bool Generate(const std::string& scd_path, const std::string& mdb_instance, const std::string& last_mdb_instance = "");


    private:

        void ProcessFurther_(Document& doc);
        ProductMatcher* GetMatcher_();


    private:
        //BrandDb* bdb_;
        int mode_;
        ProductMatcher* matcher_;
    };

}

#endif

