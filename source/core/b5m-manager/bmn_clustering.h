#ifndef SF1R_B5MMANAGER_BMNCLUSTERING_H_
#define SF1R_B5MMANAGER_BMNCLUSTERING_H_
#include "bmn_container.hpp"
#include <document-manager/ScdDocument.h>
NS_SF1R_B5M_BEGIN
class OfferDb;
//automatic brand,model,ngram clustering
class BmnClustering
{
    typedef BmnContainer Container;
    typedef BmnContainer::Value Value;
    typedef BmnContainer::ValueArray ValueArray;
    typedef boost::function<void (ScdDocument&)> Func;
    typedef uint32_t term_t;
    typedef std::vector<term_t> word_t;
    typedef std::set<word_t> KDict;

public:
    BmnClustering(const std::string& path);
    ~BmnClustering();
    void AddCategoryKeyword(const std::string& keyword);
    void AddCategoryRule(const std::string& category_regex, const std::string& group_name);
    bool IsValid(const Document& doc) const;
    bool Get(const std::string& oid, std::string& pid);
    bool Append(const Document& doc, const std::vector<std::string>& brands, const std::vector<std::string>& keywords);
    bool Finish(const Func& func, int thread_num=1);
private:
    bool GetGroupName_(const std::string& category, std::string& group_name) const;
    void TooMany_(ValueArray& va);
    void Calculate_(ValueArray& va);
    void ExtractModel_(const std::string& text, std::string& model) const;
    void GetWord_(const std::string& text, word_t& word);
    void Output_(const Value& v) const;
    Container* GetContainer_();

private:
    std::string path_;
    OfferDb* odb_;
    Container* container_;
    boost::regex model_regex_;
    std::vector<std::pair<boost::regex, std::string> > category_rule_;
    Func func_;
    KDict kdict_;
    idmlib::util::IDMAnalyzer* analyzer_;
    std::set<std::string> model_stop_set_;
    std::vector<boost::regex> error_model_regex_;
    boost::mutex mutex_;
};

NS_SF1R_B5M_END
#endif

