#include "PinYin.h"
#include <idmlib/query-correction/cn_query_correction.h>
#include <string.h>
#include <iostream>


#include "../CorrectionEngineWrapper.h"

namespace sf1r
{
namespace Recommend
{
namespace PinYin
{

QueryCorrection::QueryCorrection(const std::string dir)
    : cmgr_ (NULL)
{
    idmlib::qc::CnQueryCorrection::res_dir_ = dir;
    cmgr_ = new idmlib::qc::CnQueryCorrection(dir);
    cmgr_->Load();
}

QueryCorrection::~QueryCorrection()
{
    delete cmgr_;
}
    
QueryCorrection& QueryCorrection::getInstance()
{
    static QueryCorrection qc(CorrectionEngineWrapper::system_resource_path_ + "/query_correction/" + "/cn/");
    return qc;
}

bool QueryCorrection::getPinyin(const std::string& str, std::vector<std::string>& results) const
{
    bool ret = true;
    izenelib::util::UString ustr(str, izenelib::util::UString::UTF_8);
    cmgr_->GetPinyin2(ustr, results);
    if (!results.empty())
        ret = (str != results[0]);
    return ret;
}
    
bool QueryCorrection::getApproximatePinyin(const std::string& str, std::vector<std::string>& results) const
{
    izenelib::util::UString ustr(str, izenelib::util::UString::UTF_8);
    izenelib::util::UString nonChineseWord;
    //izenelib::util::UString chinese;
    bool isStrPinYin = true;
    for (std::size_t i = 0; i < ustr.length(); i++)
    {
        if (ustr.isChineseChar(i))
        {
            isStrPinYin &= false;
            if (nonChineseWord.length() > 0)
            {
                std::string pinyin;
                nonChineseWord.convertString(pinyin, izenelib::util::UString::UTF_8);
                approximate(pinyin, results);
                nonChineseWord.clear();
            }
            
            izenelib::util::UString uchar;
            uchar += ustr[i];
            std::vector<std::string> charList;
            cmgr_->GetPinyin(uchar, charList);
            if (charList.empty())
                continue;
            
            if (results.empty())
            {
                for (std::size_t i = 0; i < charList.size(); i++)
                {
                    results.push_back(charList[i]);
                }
            }
            else
            {
                std::size_t size = results.size();
                for (std::size_t i = 0; i < size; i++)
                {
                    for (std::size_t ii = 1; ii < charList.size(); ii++)
                    {
                        results.push_back(results[i] + charList[ii]);
                    }
                    results[i] += charList[0];
                }
            }

        }
        else
        {
            if (ustr.isSpaceChar(i))
            {
                std::string pinyin;
                nonChineseWord.convertString(pinyin, izenelib::util::UString::UTF_8);
                isStrPinYin &= approximate(pinyin, results);
                nonChineseWord.clear();

                for (std::size_t i = 0; i < results.size(); i++)
                {
                    results[i] += " ";
                }
            }
            else
                nonChineseWord += ustr[i];
        }
    }
    if (nonChineseWord.length() > 0)
    {
        std::string pinyin;
        nonChineseWord.convertString(pinyin, izenelib::util::UString::UTF_8);
        isStrPinYin &= approximate(pinyin, results);
        nonChineseWord.clear();
    }
    return isStrPinYin;
}

bool QueryCorrection::approximate(const std::string& pinyin, std::vector<std::string>& results) const
{
    std::vector<std::string> charList;
    cmgr_->FuzzySegmentRaw(pinyin, charList);
    if (!charList.empty())
    { 
        for (std::size_t i = 0; i < charList.size(); i++)
        {
            std::string normalized = "";
            for (std::size_t ii = 0; ii < charList[i].size(); ii++)
            {
                if (',' != charList[i][ii])
                    normalized += charList[i][ii];
            }
            charList[i] = normalized;
        }

        if (results.empty())
        {
            for (std::size_t i = 0; i < charList.size(); i++)
            {
                results.push_back(charList[i]);
            }
        }
        else
        {
            std::size_t size = results.size();
            for (std::size_t i = 0; i < size; i++)
            {
                for (std::size_t ii = 1; ii < charList.size(); ii++)
                {
                    results.push_back(results[i] + charList[ii]);
                }
                results[i] += charList[0];
            }
        }
        return true;
    }
    else
    {
        std::size_t size = results.size();
        if (results.empty())
        {
            results.push_back(pinyin);
        }
        for (std::size_t i = 0; i < size; i++)
        {
            results[i] += pinyin;
        }
        return false;
    }
}

}
}
}
