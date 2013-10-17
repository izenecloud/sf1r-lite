#include "CorrectionEngine.h"
#include "parser/TaobaoParser.h"
#include "StringUtil.h"

#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>    
#include <knlp/normalize.h>
#include <util/ustring/UString.h>

#include <time.h>

namespace sf1r
{
namespace Recommend
{

static std::string timestamp = ".CorrectionEngineTimestamp";

CorrectionEngine::CorrectionEngine(const std::string& workdir)
    : pinyin_(NULL)
    , prefix_(NULL)
    , filter_(NULL)
    , parsers_(NULL)
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
            //std::cout<<p<<"\n";
            if (!parsers_->isValid(p))
                continue;
            parsers_->load(p);
            std::ofstream out;
            out.open((p + ".results").c_str(), std::ofstream::out | std::ofstream::trunc);
            Parser* parser = parsers_->load(p);
            Parser::iterator it = parser->begin();
            for (; it != parser->end(); ++it)
            {
                //std::cout<<it->userQuery()<<" : "<<it->freq()<<"\n";
                std::string result;
                double freq = 0.0;
                if (correct(it->userQuery(), result, freq))
                    out<<it->userQuery()<<"\t"<<result<<"\t"<<freq<<"\n";
            }
            parsers_->destory(parser);
            out.close();
        }
    }
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
    // to do!!
    if (NULL != pyConverter_)
    {
        (*pyConverter_)(userQuery, pinyin);
    }
    for (std::size_t i = 0; i < pinyin.size(); i++)
    {
        //std::cout<<pinyin[i]<<" ";
        pinyin_->insert(userQuery, pinyin[i], freq);
    }
    //std::cout<<"\n";
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
    if (NULL != pyConverter_)
    {
        (*pyConverter_)(userQuery, pinyin);
    }

    UserQuery self(userQuery, 0);
    FreqStringVector candidates;
    
    // from pinyin
    double factor = 100;
    for (std::size_t i = 0; i < pinyin.size(); i++)
    {
        UserQueryList uqList;
        pinyin_->search(pinyin[i], uqList);
        //std::cout<<pinyin[i]<<" : ";
        UserQueryList::iterator it = uqList.begin();
        for (; it != uqList.end(); it++)
        {
            if (it->userQuery() != self.userQuery())
            {
                double s = 1.0;
                izenelib::util::UString ustr(it->userQuery(), izenelib::util::UString::UTF_8);
                if (uself.isAllChineseChar())
                {
                    if (uself.length() != ustr.length())
                        continue;
                    if (2 == uself.length())
                    {
                        if (uself[0] != ustr[0])
                        {
                            if (uself[1] != ustr[1])
                                s /= 10;
                            s /= 2;
                        }
                    }
                }
                candidates.push_back(FreqString(it->userQuery(), it->freq() * factor * s));
            }
            //std::cout<<it->userQuery()<<" ";
        }
        //std::cout<<"\n";
    }
    
    // from prefix
    factor = 1;
    UserQueryList uqList;
    bool isPrefixEnglish = PrefixTable::isPrefixEnglish(userQuery);
    prefix_->search(userQuery, uqList);
    UserQueryList::iterator it = uqList.begin();
    //std::cout<<"PREFIX::";
    for (; it != uqList.end(); it++)
    {
        if (it->userQuery() == self.userQuery())
        {
            self.setFreq(self.freq() + it->freq());
        }
        else
        {   
            if (isPrefixEnglish)
                candidates.push_back(FreqString(it->userQuery(), it->freq() * factor * 20));
            else
                candidates.push_back(FreqString(it->userQuery(), it->freq() * factor));
        }
        //std::cout<<it->userQuery()<<" ";
    }
    //std::cout<<"\n";

    factor = 1000;
    self.setFreq(self.freq() * factor);

    FreqString max = StringUtil::max(candidates);
    const double selfFreq = self.freq();
    const double maxFreq = max.getFreq();
    //std::cout<<self.freq()<<" : "<<max.getFreq()<<"\n";
    
    //for Kevin Hu
    /*bool selfHasChinese = false;
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
        const std::string& maxString = max.getString();
        izenelib::util::UString umax(maxString, izenelib::util::UString::UTF_8);
        bool maxHasChinese = false;
        for (std::size_t i = 0; i < umax.length(); i++)
        {
            if (umax.isChineseChar(i))
            {
                maxHasChinese = true;
                break;
            }
        }
        if (!maxHasChinese)
        {
            if (str.size() < maxString.size())
            {
                std::size_t i = 0;
                for (; i < maxString.size(); i++)
                {
                    if (isspace(maxString[i]))
                        continue;
                    if (std::string::npos == str.find(maxString[i]))
                        break;
                }
                if (i == maxString.size())
                {
                    results = maxString;
                    return true;
                }
            }
        }
    }*/

    //if ((2.4 * selfFreq < maxFreq) &&(maxFreq - selfFreq> 1e6))
    if (2.4 * selfFreq < maxFreq) 
    {
        izenelib::util::UString uresult(max.getString(), izenelib::util::UString::UTF_8);
        if (uresult.length() <= 1)
            return false;
        
        izenelib::util::UString selfChinese;
        for (std::size_t i = 0; i < uself.length(); i++)
        {
            if (uself.isChineseChar(i))
                selfChinese += uself[i];
        }
        
        izenelib::util::UString resultChinese;
        for (std::size_t i = 0; i < uresult.length(); i++)
        {
            if (uresult.isChineseChar(i))
                resultChinese += uresult[i];
        }
        if ((resultChinese.length() > 0) && (selfChinese == resultChinese))
            return false;
        if (max.getString().size() < self.userQuery().size())
            return false;

        results = max.getString();
        freq = maxFreq / selfFreq;
        return true;
    }
    return false;
}

}
}
