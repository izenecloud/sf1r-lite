#include "RecommendEngine.h"
#include "parser/TaobaoParser.h"

#include <knlp/fmm.h>
#include <la-manager/KNlpWrapper.h>

#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>

#include <stdlib.h>

namespace sf1r
{
namespace Recommend
{

static std::string timestamp = ".RecommendEngineTimestamp";

RecommendEngine::RecommendEngine(const std::string& dir)
    : parsers_(NULL)
    , uqcTable_(NULL)
    , indexer_(NULL)
    , timestamp_(0)
    , workdir_(dir)
{
    parsers_ = new ParserFactory();
    uqcTable_ = new UserQueryCateTable(workdir_);
    indexer_ = new IndexEngine(workdir_);
    
    std::string path = workdir_;
    path += "/";
    path += timestamp;

    std::ifstream in;
    in.open(path.c_str(), std::ifstream::in);
    if(in)
        in>>timestamp_;
    in.close();
}

RecommendEngine::~RecommendEngine()
{
    if (NULL != parsers_)
    {
        delete parsers_;
        parsers_ = NULL;
    }
    if (NULL != uqcTable_)
    {
        delete uqcTable_;
        uqcTable_ = NULL;
    }
    if (NULL != indexer_)
    {
        delete indexer_;
        indexer_ = NULL;
    }
}

void RecommendEngine::evaluate(const std::string& path) const
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
                if ((rand() % 100) >= 11)
                    continue;

                const uint32_t N = 10;
                std::vector<std::string> results;
                recommend(it->userQuery(), N, results);
                out<<it->userQuery()<<"\t";
                for (std::size_t i = 0; i < results.size(); i++)
                    out<<results[i]<<";";
                out<<"\n";
                out.flush();
            }
            out.close();
            delete parser;
        }
    }
}

void RecommendEngine::recommend(const std::string& userQuery, const uint32_t N, std::vector<std::string>& results) const
{
    recommend_(userQuery, N, results);
    if (results.size() < N)
    {
        static izenelib::util::UString U_SPACE(" ", izenelib::util::UString::UTF_8);
        izenelib::util::UString u(userQuery, izenelib::util::UString::UTF_8);
        if (u.length() < 2)
            return;
        izenelib::util::UString uInSpace;
        for (std::size_t i = 0; i < u.length(); i++)
        {
            if ( 0 == i % 2)
                uInSpace += U_SPACE;
            uInSpace += u[i];
        }
        std::string inSpace; 
        uInSpace.convertString(inSpace, izenelib::util::UString::UTF_8);
        recommend_(inSpace, N - results.size(), results);
    }
}

void RecommendEngine::recommend_(const std::string& userQuery, const uint32_t N, std::vector<std::string>& results) const
{
    static CateEqualer equaler = boost::bind(&UserQueryCateTable::cateEqual, uqcTable_, _1, _2);
    
    FreqStringVector byCate;
    FreqStringVector byFreq;
    indexer_->search(userQuery, byCate, byFreq, N, &equaler);
   
    StringUtil::tuneByEditDistance(byCate, userQuery);
    StringUtil::tuneByEditDistance(byFreq, userQuery);

    uint32_t n = 0;
    std::size_t i = 0;
    for (; i < byCate.size(); i++)
    {
        results.push_back(byCate[i].getString());
        if (++n >= 3 * N / 4)
            break;
    }
    
    i++;
    std::size_t j = 0;
    while (true)
    {
        if ((i >= byCate.size()) && (j >= byFreq.size()))
            break;
        FreqString cateItem("", 0.0);
        if (i < byCate.size())
            cateItem = byCate[i];
            
        FreqString freqItem("", 0.0);
        if (j < byFreq.size())
            freqItem = byFreq[j];

        if (freqItem.getFreq() > cateItem.getFreq())
        {
            {
                results.push_back(freqItem.getString());
                n++;
            }
            j++;
        }
        else
        {
            {
                results.push_back(cateItem.getString());
                n++;
            }
            i++;
        }

        if (n >= N)
            break;
    }

    /*FreqStringVector uqByFreq;
    for (std::size_t i = 0; i < candidates.size(); i++)
    {
        if (StringUtil::isNeedRemove(candidates[i].getString(), nUserQuery))
            continue;
        uqByFreq.push_back(candidates[i]);
        if (uqByFreq.size() >= N)
            break;
    }

    //StringUtil::removeItem(uqByFreq, nUserQuery);
    StringUtil::tuneByEditDistance(uqByFreq, nUserQuery);
    for (std::size_t i = 0; (i < uqByFreq.size()) && (i < N); i++)
    {
        results.push_back(uqByFreq[i].getString());
    }
    uqByFreq.clear();*/
    // todo category information.
    /*
    TermCateTable::CateIdList cateIds; // category list for userQuery
    Tokenize::TermVector::iterator it = tv.begin();
    for (; it != tv.end(); ++it)
    {
        TermCateTable::CateIdList ids;
        tcTable_->category(it->term(), ids);
        //std::cout<<ids;
        cateIds += ids;
        //std::cout<<cateIds;
    }
    
    cateIds.sort();
    cateIds.reverse();
    TermCateTable::CateIdList::iterator id_it = cateIds.begin();
    for (; id_it != cateIds.end(); id_it++)
    {
        std::cout<<tcTable_->cateIdToCate(id_it->first)<<":"<<id_it->second<<";";
    }
    std::cout<<"\n";

    FreqStringVector uqByCate;
    uint32_t n = 0;
    for (id_it = cateIds.begin(); id_it != cateIds.end(); id_it++)
    {
        UserQueryCateTable::UserQueryList userQuery;
        uqcTable_->topNUserQuery(tcTable_->cateIdToCate(id_it->first), N / 2, userQuery);
        UserQueryCateTable::UserQueryList::iterator uq_it = userQuery.begin();
        std::cout<<tcTable_->cateIdToCate(id_it->first)<<":";
        for(; uq_it != userQuery.end(); uq_it++)
        {
            std::cout<<uq_it->userQuery()<<" ";
            uqByCate.push_back(FreqString(uq_it->userQuery(), uq_it->freq()));
            if (++n >= 2 *N)
                break;
        }
        std::cout<<"\n";
        if ( n>= 2*N)
            break;
    }
    StringUtil::removeDuplicate(uqByCate);
    std::sort(uqByCate.begin(), uqByCate.end());
    
    StringUtil::tuneByEditDistance(uqByCate, userQuery);
    for (std::size_t i = 0; i < uqByCate.size(); i++)
    {
        results.push_back(uqByCate[i].getString());
    }
    */ 
}

void RecommendEngine::flush() const
{
    indexer_->flush();
    //tcTable_->flush();
    uqcTable_->flush();
    
    std::string path = workdir_;
    path += "/";
    path += timestamp;

    std::ofstream out;
    out.open(path.c_str(), std::ofstream::out | std::ofstream::trunc);
    out<<timestamp_;
    out.close();
}

void RecommendEngine::clear()
{
    indexer_->clear();
}

bool RecommendEngine::isNeedBuild(const std::string& path) const
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
    return false;
}

void RecommendEngine::buildEngine(const std::string& path)
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
            Parser::iterator it = parser->begin();
            for (; it != parser->end(); ++it)
            {
                //std::cout<<it->userQuery()<<" : "<<it->freq()<<"\n";
                KNlpWrapper::string_t kstr(it->userQuery());
                ilplib::knlp::Normalize::normalize(kstr);
                std::string nUserQuery = kstr.get_bytes("utf-8");
                /*std::string nUserQuery = "";
                for (std::size_t i = 0; i < str.size(); i++)
                {
                    if (!isspace(str[i]))
                        nUserQuery += str[i];
                }*/
                processQuery(nUserQuery, it->category(), it->freq());
            }
            delete parser;
        }
    }

    timestamp_ = time(NULL);
    flush();
}

void RecommendEngine::processQuery(const std::string& userQuery, const std::string& category, const uint32_t freq)
{
    //Tokenize::TermVector tv;
    //(*tokenizer_)(userQuery, tv);

    //Tokenize::TermVector::iterator it = tv.begin();
    //for (; it != tv.end(); ++it)
    //{
        //std::cout<<it->term()<<" ";
        //tcTable_->insert(it->term(), category, freq); 
    //}
    //std::cout<<" : "<<category<<" : "<<freq<<"\n";

    uqcTable_->insert(userQuery, category, freq);
    indexer_->insert(userQuery, freq);
}

}
}
