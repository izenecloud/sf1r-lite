#include "product_term_analyzer.h"
#include <la-manager/LAPool.h>
using namespace sf1r;

// #define PM_CLUST_TEXT_DEBUG

ProductTermAnalyzer::ProductTermAnalyzer()
{
    std::string kma_path;
    LAPool::getInstance()->get_kma_path(kma_path );
    std::string cma_path;
    LAPool::getInstance()->get_cma_path(cma_path );
    std::string jma_path;
    LAPool::getInstance()->get_jma_path(jma_path );
//     idmlib::util::IDMAnalyzerConfig aconfig = idmlib::util::IDMAnalyzerConfig::GetCommonConfig(kma_path,cma_path,jma_path);
    //use cma only
    idmlib::util::IDMAnalyzerConfig aconfig = idmlib::util::IDMAnalyzerConfig::GetCommonConfig("",cma_path,"");
    aconfig.symbol = true;
    aconfig.cma_config.merge_alpha_digit = true;
    analyzer_ = new idmlib::util::IDMAnalyzer(aconfig);
}

ProductTermAnalyzer::~ProductTermAnalyzer()
{
    delete analyzer_;
}

void ProductTermAnalyzer::Analyze(const izenelib::util::UString& title, std::vector<std::string>& terms, std::vector<double>& weights)
{
    std::vector<idmlib::util::IDMTerm> term_list;
    analyzer_->GetTermList(title, term_list);
    
    
    terms.reserve(term_list.size());
    weights.reserve(term_list.size());

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
    
    TERM_STATUS status = NORMAL;
    
    for (uint32_t i=0;i<term_list.size();i++)
    {
        const std::string& str = term_list[i].TextString();
        char tag = term_list[i].tag;
        double weight = GetWeight_(title, term_list[i].text, tag);
        if( weight<=0.0 ) continue;
        terms.push_back(str);
        weights.push_back(weight);
    }
    
#ifdef PM_CLUST_TEXT_DEBUG
    std::cout<<"[ATokens] ";
    for (uint32_t i=0;i<terms.size();i++)
    {
        std::cout<<"["<<terms[i]<<","<<weights[i]<<"],";
    }
    std::cout<<std::endl;
#endif

}

double ProductTermAnalyzer::GetWeight_(const izenelib::util::UString& all, const izenelib::util::UString& term, char tag)
{
    if(tag=='.') return 0.0;
    double weight = 1.0;
    uint32_t term_length = term.length();
    bool is_model = false;
    bool has_alpha = false;
    bool has_digit = false;
    if(tag=='F')
    {
        for(uint32_t i=0;i<term_length;i++)
        {
            if(term.isAlphaChar(i))
            {
                has_alpha = true;
            }
            else if(term.isDigitChar(i))
            {
                has_digit = true;
            }
        }
    }
    if(has_alpha && has_digit)
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
    
    double length_factor = std::log( (double)all.length());
    if(length_factor<1.0) length_factor = 1.0;
    if(is_model)
    {
        weight = 0.5*length_factor;
    }
    else if(tag=='F')
    {
        if(term.length()<2)
        {
            weight = 0.0;
        }
        else
        {
            weight = 1;
        }
    }
    else if(tag=='M')
    {
        weight = 0.3*term.length();
    }
    return weight;
}

