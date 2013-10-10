#include "PinYin.h"
#include <idmlib/query-correction/cn_query_correction.h>
#include <string.h>
#include <iostream>

#include "../CorrectionEngineWrapper.h"

namespace sf1r
{
namespace Recommend
{
/*static unsigned int nWord(const std::string& str)
{
    const char* pos = str.c_str();
    const char* end = pos + str.size();
    unsigned int n = 0;
    for(; pos < end; pos++)
    {
        if (*pos == ',')
            n++;
    }
    n++;
    return n;
}*/
class QueryCorrection
{
public:
    QueryCorrection(const std::string dir)
    {
        idmlib::qc::CnQueryCorrection::res_dir_ = dir;
        cmgr_ = new idmlib::qc::CnQueryCorrection(dir);
        cmgr_->Load();
    }

    ~QueryCorrection()
    {
        delete cmgr_;
    }
public:
    void GetPinyin(const std::string& str, std::vector<std::string>& results)
    {
        // deal with blank, character, number.
        //std::list<std::vector<std::string> > pinyins;
        /*const char* pos = str.c_str();
        const char* end = pos + str.size();
        std::string chars = "";
        std::string chinese = "";
        for(; pos < end; pos++)
        {
            if (isalpha(*pos) || isdigit(*pos))
            {
                chars += *pos;
            }
            else if((!isblank(*pos)) && (!isspace(*pos)))
            {
                chinese += *pos;
            }
        }
        if (chinese.empty())
        {
            if (!chars.empty())
                results.push_back(chars);
            return;
        }*/
        izenelib::util::UString ustr(str, izenelib::util::UString::UTF_8);
        cmgr_->GetPinyin2(ustr, results);

        /*std::vector<std::string> pinyin;
        cmgr_->GetPinyin2(ustr, pinyin);
        for (std::size_t i = 0; i < pinyin.size(); i++)
        {
            std::cout<<pinyin[i]<<"\n";
        }
        if (pinyin.empty())
        {
            if (!chars.empty())
                results.push_back(chars);
            return;
        }
        else if (ustr.length() != nWord(pinyin[0]))
        {
            std::cout<<pinyin[0]<<"\n";
            std::cout<<ustr.length()<<" : "<<nWord(pinyin[0])<<"\n";
            results.clear();
            return;
        }

        chars += ",";
        for (std::size_t i = 0; i < pinyin.size(); i++)
        {
            if ("," == chars)
                results.push_back(pinyin[i]);
            else
                results.push_back(chars + pinyin[i]);
        }*/

        //std::list<std::vector<std::string> >::iterator it = pinyins.begin();
        //for (; it != pinyins.end(); it++)
        //{
        //}
        
    }

private:
    idmlib::qc::CnQueryCorrection* cmgr_;
};

PinYinConverter* getPinYinConverter()
{
    static QueryCorrection* qc = new QueryCorrection(CorrectionEngineWrapper::system_resource_path_ + "/query_correction/" + "/cn/");
    static PinYinConverter converter = boost::bind(&QueryCorrection::GetPinyin, qc, _1, _2);
    return &converter;
}
}
}
