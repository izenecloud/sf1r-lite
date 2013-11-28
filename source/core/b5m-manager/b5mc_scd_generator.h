#ifndef SF1R_B5MMANAGER_B5MCSCDGENERATOR_H_
#define SF1R_B5MMANAGER_B5MCSCDGENERATOR_H_

#include "b5m_types.h"
#include <string>
#include <vector>
//#include "brand_db.h"
#include "product_matcher.h"
#include "comment_db.h"
#include "offer_db.h"
#include "offer_db_recorder.h"
#include "b5m_m.h"
#include <document-manager/ScdDocument.h>

NS_SF1R_B5M_BEGIN
class B5mcScdGenerator {
public:
    B5mcScdGenerator(const B5mM& b5mm);
    ~B5mcScdGenerator();

    bool Generate(const std::string& mdb_instance, const std::string& last_mdb_instance = "");

    void Process(ScdDocument& doc);

private:

    void ProcessFurther_(ScdDocument& doc);
    ProductMatcher* GetMatcher_();


private:
    //BrandDb* bdb_;
    B5mM b5mm_;
    int mode_;
    ProductMatcher* matcher_;
    boost::shared_ptr<CommentDb> cdb_;
    boost::shared_ptr<OfferDbRecorder> odb_;
    std::size_t new_count_;
    std::size_t pid_changed_count_;
    std::size_t pid_not_changed_count_;
    boost::mutex mutex_;
};

NS_SF1R_B5M_END

#endif

