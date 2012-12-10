#ifndef SF1R_B5MMANAGER_B5MCSCDGENERATOR_H_
#define SF1R_B5MMANAGER_B5MCSCDGENERATOR_H_

#include <string>
#include <vector>
#include "comment_db.h"
#include "offer_db.h"
#include "offer_db_recorder.h"
#include "brand_db.h"
#include "product_matcher.h"

namespace sf1r {
    class B5mcScdGenerator {
    public:
        B5mcScdGenerator(CommentDb* cdb, OfferDbRecorder* odb, BrandDb* bdb, ProductMatcher* matcher, int mode);

        bool Generate(const std::string& scd_path, const std::string& mdb_instance);


    private:

        void ProcessFurther_(Document& doc);
        ProductMatcher* GetMatcher_();


    private:
        CommentDb* cdb_;
        OfferDbRecorder* odb_;
        BrandDb* bdb_;
        ProductMatcher* matcher_;
        int mode_;
    };

}

#endif

