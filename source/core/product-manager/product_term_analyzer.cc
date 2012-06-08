#include "product_term_analyzer.h"
#include <la-manager/LAPool.h>


using namespace sf1r;

//#define PM_CLUST_TEXT_DEBUG

ProductTermAnalyzer::ProductTermAnalyzer(const std::string& pcma_path)
{
    std::string cma_path = pcma_path;
    if(cma_path.empty())
    {
        LAPool::getInstance()->get_cma_path(cma_path );
    }
    //use cma only
    idmlib::util::IDMAnalyzerConfig aconfig = idmlib::util::IDMAnalyzerConfig::GetCommonConfig("",cma_path,"");
    aconfig.cma_config.merge_alpha_digit = true;
    analyzer_ = new idmlib::util::IDMAnalyzer(aconfig);

    //make stop words manually here
    stop_set_.insert("的");
    stop_set_.insert("在");
    stop_set_.insert("是");
    stop_set_.insert("和");
}

ProductTermAnalyzer::~ProductTermAnalyzer()
{
    delete analyzer_;
}

void ProductTermAnalyzer::Analyze(const izenelib::util::UString& title, std::vector<std::pair<std::string, double> >& doc_vector)
{
    std::vector<idmlib::util::IDMTerm> term_list;
    analyzer_->GetTermList(title, term_list);

    doc_vector.reserve(term_list.size());

#ifdef PM_CLUST_TEXT_DEBUG
    std::string stitle;
    title.convertString(stitle, izenelib::util::UString::UTF_8);
    std::cout<<"[Title] "<<stitle<<std::endl<<"[OTokens] ";
    for (uint32_t i=0;i<term_list.size();i++)
    {
        std::cout<<"["<<term_list[i].TextString()<<","<<term_list[i].tag<<"],";
    }
    std::cout<<std::endl;
#endif
    if(term_list.empty()) return;
//     TERM_STATUS status = NORMAL;

    boost::unordered_set<std::string> app;

    for (uint32_t i=0;i<term_list.size();i++)
    {
        const std::string& str = term_list[i].TextString();
        if( stop_set_.find(str)!=stop_set_.end()) continue;
        if( app.find(str)!=app.end()) continue;
        app.insert(str);
        char tag = term_list[i].tag;
        double weight = GetWeight_(term_list.size(), term_list[i].text, tag);
        if( weight<=0.0 ) continue;
        doc_vector.push_back(std::make_pair(str, weight));
    }

#ifdef PM_CLUST_TEXT_DEBUG
    std::cout<<"[ATokens] ";
    for (uint32_t i=0;i<doc_vector.size();i++)
    {
        std::cout<<"["<<doc_vector[i].first<<","<<doc_vector[i].second<<"],";
    }
    std::cout<<std::endl;
#endif

}

double ProductTermAnalyzer::GetWeight_(uint32_t title_length, const izenelib::util::UString& term, char tag)
{
    if(tag=='.') return 0.0;
    double weight = 1.0;
    uint32_t term_length = term.length();
    bool is_model = false;
    if(tag=='X')
    {
        is_model = true;
    }
    else
    {
        if(tag=='M' && term_length>=4)
        {
            is_model = true;
        }
    }

    if(is_model)
    {
        double length_factor = std::log( (double)title_length*3);
        weight = 0.5*length_factor;
        if( weight<1.0) weight = 1.0;
//         else if(weight>2.0) weight = 2.0;
    }

    else if(tag=='F')
    {
        if(term.length()<2)
        {
            izenelib::util::UString lower(term);
            lower.toLowerString();
            if(term==lower)
            {
                weight = 0.0;
            }
            else
            {
                //single capital character
                weight = 1.0;
            }
        }
        else
        {
            weight = 1.0;
        }
    }
    else if(tag=='M')
    {
        weight = 0.3*term.length();
    }
    return weight;
}
