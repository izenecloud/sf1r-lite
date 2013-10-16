#include "CorrectionEngine.h"
#include "parser/TaobaoParser.h"
#include "StringUtil.h"
#include "evaluate/EvaluatorFactory.h"

#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>    
#include <knlp/normalize.h>
#include <util/ustring/UString.h>

#include <time.h>
#include <stdlib.h>
#include <ctype.h>

namespace sf1r
{
namespace Recommend
{

static double factorForPinYin(const std::string& self, const std::string& userQuery)
{
    izenelib::util::UString uself(self, izenelib::util::UString::UTF_8);
    izenelib::util::UString ustr(userQuery, izenelib::util::UString::UTF_8);
    double s = 1.0;
    if (uself.isAllChineseChar())
    {
        if (uself.length() != ustr.length())
            return 1e-6;
        if (2 == uself.length())
        {
            if (uself[0] != ustr[0])
            {
                if (uself[1] != ustr[1])
                    s /= 10;
                s /= 2;
            }
            else
                s *= 1.28;
        }
        if (2 < uself.length())
        {
            uint32_t n = 0;
            for (std::size_t i = 0; i < uself.length(); i++)
            {
                if (uself[i] == ustr[i])
                    n++;
            }
            s *= n;
            s /= (uself.length() -n);
        }
    }
    return s;
}

static double factorForPrefix(const std::string& self, const std::string& userQuery)
{
    izenelib::util::UString uself(self, izenelib::util::UString::UTF_8);
    izenelib::util::UString ustr(userQuery, izenelib::util::UString::UTF_8);
    double s = 1.0;
    if (PrefixTable::isPrefixEnglish(userQuery))
        s *= 20;
   
    // iphone5 => iphone 5
    bool selfHasChinese = false;
    for (std::size_t i = 0; i < uself.length(); i++)
    {
        if (uself.isChineseChar(i))
        {
            selfHasChinese = true;
            break;
        }
    }
    if (!selfHasChinese)
    {
        bool ustrHasChinese = false;
        for (std::size_t i = 0; i < ustr.length(); i++)
        {
            if (ustr.isChineseChar(i))
            {
                ustrHasChinese = true;
                break;
            }
        }
        if (!ustrHasChinese)
        {
            std::size_t selfL = self.size();
            std::size_t userL = userQuery.size();
            if (selfL < userL)
            {
                std::size_t i = 0, j = 0;
                while (true)
                {
                    if ((i >= selfL) || (j >= userL))
                        break;
                    if (self[i] == userQuery[j])
                    {
                        i++;
                        j++;
                    }
                    else if (' ' == self[i])
                    {
                        break;
                    }
                    else if (' ' == userQuery[j])
                    {
                        j++;
                    }
                    else 
                    {
                        break;
                    }
                }
                if ((i == selfL) && (j == userL))
                    s *= 400;
            }
        }
    }

    // digit change iphone6 => iphone5
    std::size_t selfL = uself.length();
    std::size_t ustrL = ustr.length();
    std::size_t i = 0;
    std::size_t j = 0;
    while (true)
    {
        if ((i >= selfL) || (j >= ustrL))
            break;
        if (uself[i] == ustr[j])
        {
            i++;
            j++;
        }
        else if (uself.isSpaceChar(i))
        {
            i++;
        }
        else if (ustr.isSpaceChar(j))
        {
            j++;
        }
        else if (uself.isDigitChar(i))
        {
            if (ustr.isDigitChar(j))
                s /= 100;
            else if (ustr.isChineseChar(j))
                s /= 200;
            else
                s /= 150;
            i++;
            j++;
        }
        else if (ustr.isDigitChar(j))
        {
            if (uself.isDigitChar(i))
                s /= 100;
            else if (uself.isChineseChar(i))
                s /= 200;
            else
                s /= 150;
            i++;
            j++;
        }
        else
        {
            i++;
            j++;
        }
    }
    if ((i < selfL) && (uself.isDigitChar(i)))
        s /= 150;
    if (selfL >= ustrL)
        s /= 32;

    std::size_t nself = 0;
    for (i = 0; i < selfL; i++)
    {
        if (!uself.isSpaceChar(i))
            nself++;
    }
    std::size_t nstr = 0;
    for (i = 0; i < ustrL; i++)
    {
        if (!ustr.isSpaceChar(i))
            nstr++;
    }
    if (nself != nstr)
        s /= 1024;
    return s; 
}

static std::string timestamp = ".CorrectionEngineTimestamp";

CorrectionEngine::CorrectionEngine(const std::string& workdir)
    : pinyin_(NULL)
    , prefix_(NULL)
    , filter_(NULL)
    , parsers_(NULL)
    , pyConverter_(NULL)
    , pyApproximator_(NULL)
    , timestamp_(0)
    , workdir_(workdir)
{
    if (!boost::filesystem::exists(workdir_))
    {
        boost::filesystem::create_directory(workdir_);
    }
    
    pinyin_ = new PinyinTable(workdir_);
    prefix_ = new PrefixTable(workdir_);
    filter_ = new Filter(workdir_ + "/filter/");
    parsers_ = new ParserFactory();
    
    std::string path = workdir_;
    path += "/";
    path += timestamp;

    std::ifstream in;
    in.open(path.c_str(), std::ifstream::in);
    in>>timestamp_;
    in.close();
}

CorrectionEngine:: ~CorrectionEngine()
{
    if (NULL != pinyin_)
    {
        delete pinyin_;
        pinyin_ = NULL;
    }
    if (NULL != prefix_)
    {
        delete prefix_;
        prefix_ = NULL;
    }
    if (NULL != filter_)
    {
        delete filter_;
        filter_ = NULL;
    }
    if (NULL != parsers_)
    {
        delete parsers_;
        parsers_ = NULL;
    }
}
    
void CorrectionEngine::evaluate(const std::string& path) const
{
    std::string resource;
    if (!boost::filesystem::exists(path))
        resource = workdir_;
    else if(!boost::filesystem::is_directory(path))
        resource = workdir_;
    else
        resource = path;
    
    boost::filesystem::directory_iterator end;
    for(boost::filesystem::directory_iterator it(resource) ; it != end ; ++it)
    {
        const std::string p = it->path().string();
        if(boost::filesystem::is_regular_file(p))
        {
            if (!EvaluatorFactory::isValid(p))
                continue;
            
            Evaluator* evaluator = EvaluatorFactory::load(p);
            Evaluator::iterator it = evaluator->begin();
            for (; it != evaluator->end(); ++it)
            {
                std::string result;
                double freq = 0.0;
                correct(it->userQuery(), result, freq);
                evaluator->isCorrect(result);
            }
            EvaluatorFactory::destory(evaluator);
        }
    }
    std::string stream;
    Evaluator::toString(stream);
    std::cout<<stream<<"\n";
}

bool CorrectionEngine::isNeedBuild(const std::string& path) const
{
    std::string resource;
    if (!boost::filesystem::exists(path))
        resource = workdir_;
    else if(!boost::filesystem::is_directory(path))
        resource = workdir_;
    else
        resource = path;
    
    boost::filesystem::directory_iterator end;
    for(boost::filesystem::directory_iterator it(resource) ; it != end ; ++it)
    {
        const std::string p = it->path().string();
        if(boost::filesystem::is_regular_file(p))
        {
            //std::cout<<p<<"\n";
            if (!parsers_->isValid(p))
                continue;
            std::time_t mt = boost::filesystem::last_write_time(it->path());
            if (mt > timestamp_)
                return true;
        }
    }
    return filter_->isNeedBuild();
}

void CorrectionEngine::buildEngine(const std::string& path)
{
    clear();

    std::string resource;
    if (!boost::filesystem::exists(path))
        resource = workdir_;
    else if(!boost::filesystem::is_directory(path))
        resource = workdir_;
    else
        resource = path;
    
    boost::filesystem::directory_iterator end;
    for(boost::filesystem::directory_iterator it(resource) ; it != end ; ++it)
    {
        const std::string p = it->path().string();
        if(boost::filesystem::is_regular_file(p))
        {
            //std::cout<<p<<"\n";
            if (!parsers_->isValid(p))
                continue;
            Parser* parser = parsers_->load(p);
            if (NULL == parser)
                continue;
            Parser::iterator it = parser->begin();
            for (; it != parser->end(); ++it)
            {
                //std::cout<<it->userQuery()<<" : "<<it->freq()<<"\n";
                processQuery(it->userQuery(), it->freq());
            }
            parsers_->destory(parser);
        }
    }
    
    filter_->buildFilter();
    timestamp_ = time(NULL);
    flush();
}

void CorrectionEngine::processQuery(const std::string& str, const uint32_t freq)
{
    std::string userQuery = str;
    try
    {
        ilplib::knlp::Normalize::normalize(userQuery);
    }
    catch(...)
    {
    }
    
    std::vector<std::string> pinyin;
    if (NULL != pyConverter_)
    {
        (*pyConverter_)(userQuery, pinyin);
    }
    for (std::size_t i = 0; i < pinyin.size(); i++)
    {
        pinyin_->insert(userQuery, pinyin[i], freq);
    }
    
    prefix_->insert(userQuery, freq);
}

void CorrectionEngine::clear()
{
    pinyin_->clear();
    prefix_->clear();
    filter_->clear();
}

void CorrectionEngine::flush() const
{
    pinyin_->flush();
    prefix_->flush();
    filter_->flush();
    
    std::string path = workdir_;
    path += "/";
    path += timestamp;

    std::ofstream out;
    out.open(path.c_str(), std::ofstream::out | std::ofstream::trunc);
    out<<timestamp_;
    out.close();
}

bool CorrectionEngine::correct(const std::string& str, std::string& results, double& freq) const
{
    std::string userQuery = str;
    try
    {
        ilplib::knlp::Normalize::normalize(userQuery);
    }
    catch(...)
    {
    }
    izenelib::util::UString uself(userQuery, izenelib::util::UString::UTF_8);
    if (uself.length() <= 1)
        return false;

    if (filter_->isFilter(userQuery))
    {
        //std::cout<<"in filter.\n";
        return false;
    }
    std::vector<std::string> pinyin;
    bool isUserQueryHasChinese = false;
    if (NULL != pyConverter_)
    {
        isUserQueryHasChinese = (*pyConverter_)(userQuery, pinyin);
    }

    UserQuery self(userQuery, 0);
    FreqStringVector candidates;
     
    // from pinyin
    double factor = 100;
    boost::unordered_map<std::string, bool> accurate;
    for (std::size_t i = 0; i < pinyin.size(); i++)
    {
        UserQueryList uqList;
        pinyin_->search(pinyin[i], uqList);
        accurate[pinyin[i]] = true;
//        std::cout<<pinyin[i]<<"\t-------------\n";
        UserQueryList::iterator it = uqList.begin();
        for (; it != uqList.end(); it++)
        {
            if (it->userQuery() != self.userQuery())
            {
                double s = factorForPinYin(self.userQuery(), it->userQuery());
        //        std::cout<<it->userQuery()<<" : "<<it->freq()<<" : "<<s<<"\n";
                candidates.push_back(FreqString(it->userQuery(), it->freq() * factor * s));
            }
            //std::cout<<it->userQuery()<<" ";
        }
        //std::cout<<"\n";
    }
   
    // from approximate pinyin
    pinyin.clear();
    bool isUserQueryPinYin = false;
    //std::string xx = isUserQueryHasChinese? "chinese" : "no chinese";
    //std::cout<<xx<<"\n";
    if ((NULL != pyApproximator_) && (candidates.empty() || (!isUserQueryHasChinese)))
    {
        isUserQueryPinYin = (*pyApproximator_)(userQuery, pinyin);
        factor = 50;
        for (std::size_t i = 0; i < pinyin.size(); i++)
        {
            UserQueryList uqList;
            if (accurate.end() != accurate.find(pinyin[i]))
                continue;
            pinyin_->search(pinyin[i], uqList);
      //      std::cout<<pinyin[i]<<"\t-------------\n";
            UserQueryList::iterator it = uqList.begin();
            for (; it != uqList.end(); it++)
            {
                if (it->userQuery() != self.userQuery())
                {
                    double s = factorForPinYin(self.userQuery(), it->userQuery());
    //            std::cout<<it->userQuery()<<" : "<<it->freq()<<" : "<<s<<"\n";
                    candidates.push_back(FreqString(it->userQuery(), it->freq() * factor * s));
                }
            }
        }
    }
    pinyin.clear();
    accurate.clear();

    // from prefix
    factor = 1;
    UserQueryList uqList;
    prefix_->search(userQuery, uqList);
    UserQueryList::iterator it = uqList.begin();
  //  std::cout<<"PREFIX::\t-----------\n";
    for (; it != uqList.end(); it++)
    {
        if (it->userQuery() == self.userQuery())
        {
            self.setFreq(self.freq() + it->freq());
        }
        else
        {
            double s = factorForPrefix(self.userQuery(), it->userQuery());
     //       std::cout<<it->userQuery()<<" : "<<s<<"\n";
            candidates.push_back(FreqString(it->userQuery(), it->freq() * factor * s));
        }
    }

    factor = 1000;
    double s = isUserQueryPinYin ? 0.1 : 1;
    self.setFreq(self.freq() * factor * s);
    if (self.freq() < 1e-2)
    {
        self.setFreq(100);
    }

    FreqString max = StringUtil::max(candidates);
    const double selfFreq = self.freq();
    const double maxFreq = max.getFreq();
    
   // std::cout<<self.freq()<<"\t:\t"<<max.getFreq()<<"\n";
   // std::cout<<self.userQuery()<<"\t:\t"<<max.getString()<<"\n";
    //if ((2.4 * selfFreq < maxFreq) &&(maxFreq - selfFreq> 1e6))
    if (2.4 * selfFreq < maxFreq)
    {
        results = max.getString();
        freq = maxFreq / selfFreq;
        return true;
    }
    return false;
}

}
}
