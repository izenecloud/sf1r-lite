#ifndef SF1R_B5MMANAGER_BMNTESTER_HPP_
#define SF1R_B5MMANAGER_BMNTESTER_HPP_

#include "bmn_clustering.h"
#include "product_matcher.h"
#include "original_mapper.h"
#include "scd_doc_processor.h"
NS_SF1R_B5M_BEGIN

class BmnTester
{
public:
    BmnTester(): doc_limit_(0)
    {
    }
    void SetDocLimit(std::size_t l) {doc_limit_ = l;}
    void Process(const std::string& knowledge, const std::string& scd)
    {
        static const int thread_num = 5;
        om_ = new OriginalMapper;
        om_->OpenFile("/opt/codebase/b5m-operation/omapper_data");
        matcher_ = new ProductMatcher;
        if(!matcher_->Open(knowledge))
        {
            LOG(ERROR)<<"matcher open failed"<<std::endl;
            return;
        }
        std::string path="./bmn_test";
        if(boost::filesystem::exists(path))
        {
            boost::filesystem::remove_all(path);
        }
        boost::filesystem::create_directories(path);
        bc_ = new BmnClustering(path);
        ScdDocProcessor processor(boost::bind(&BmnTester::DocProcess, this, _1), thread_num);
        if(doc_limit_>0)
        {
            processor.SetLimit(doc_limit_);
        }
        processor.AddInput(scd);
        processor.Process();
        bc_->Finish(boost::bind(&BmnTester::Finish, this, _1));
        delete bc_;
        delete om_;
        delete matcher_;
    }
    void DocProcess(ScdDocument& doc)
    {
        std::string source;
        doc.getString("Source", source);
        std::string ocategory;
        doc.getString("OriginalCategory", ocategory);
        std::string category;
        if(source.empty()||ocategory.empty()) return;
        om_->Map(source, ocategory, category);
        if(category.empty()) return;
        doc.property("Category") = str_to_propstr(category);
        if(!bc_->IsValid(doc)) return;
        ProductMatcher::KeywordVector kv;
        UString title;
        doc.getString("Title", title);
        matcher_->GetKeywords(title, kv);
        std::vector<std::string> brands;
        std::vector<std::string> ckeywords;
        for(std::size_t k=0;k<kv.size();k++)
        {
            std::string str;
            kv[k].text.convertString(str, UString::UTF_8);
            if(matcher_->IsBrand(category, kv[k]))
            {
                brands.push_back(str);
            }
            else if(kv[k].IsCategoryKeyword())
            {
                ckeywords.push_back(str);
            }
        }
        if(brands.empty()) return;
        std::sort(ckeywords.begin(), ckeywords.end());
        bc_->Append(doc, brands, ckeywords);
    }
    void Finish(Document& doc)
    {
    }
private:
    OriginalMapper* om_;
    ProductMatcher* matcher_;
    BmnClustering* bc_;
    std::size_t doc_limit_;
};

NS_SF1R_B5M_END
#endif

