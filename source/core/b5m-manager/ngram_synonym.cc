#include "ngram_synonym.h"

using namespace sf1r;

NgramSynonym::NgramSynonym()
: gb_regex_("^\\d{1,3}g$")
{
}

bool NgramSynonym::Get(const izenelib::util::UString& ngram, std::vector<izenelib::util::UString>& synonym)
{
    std::string str;
    ngram.convertString(str, izenelib::util::UString::UTF_8);
    if(boost::regex_match(str, gb_regex_))
    {
        if(str!="3g" && str!="2g" && str !="1g")
        {
            std::string syn = str+"b";
            synonym.push_back(izenelib::util::UString(syn, izenelib::util::UString::UTF_8));
            //std::cout<<"[NS] "<<str<<","<<syn<<std::endl;
            return true;
        }
    }
    return false;
}

