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

RecommendEngine::RecommendEngine(const std::string& dir)
    : tokenizer_(NULL)
    , parsers_(NULL)
    , tcTable_(NULL)
    , uqcTable_(NULL)
    , indexer_(NULL)
    , workdir_(dir)
{
    parsers_ = new ParserFactory();
    //tcTable_ = new TermCateTable(workdir_);
    //uqcTable_ = new UserQueryCateTable(workdir_);
    indexer_ = new IndexEngine(workdir_);
}

RecommendEngine::~RecommendEngine()
{
    if (NULL != parsers_)
    {
        delete parsers_;
        parsers_ = NULL;
    }
    if (NULL != tcTable_)
    {
        delete tcTable_;
        tcTable_ = NULL;
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

                const uint32_t N = 6;
                std::vector<std::string> results;
                recommend(it->userQuery(), N, results);
                out<<it->userQuery()<<"\t";
                for (std::size_t i = 0; i < results.size(); i++)
                    out<<results[i]<<";";
                out<<"\n";
                out.flush();
            }
            out.close();
        }
    }
}

void RecommendEngine::recommend(const std::string& userQuery, const uint32_t N, std::vector<std::string>& results) const
{
    if (NULL == tokenizer_)
        return;
    
    KNlpWrapper::string_t kstr(userQuery);
    ilplib::knlp::Normalize::normalize(kstr);
    std::string str = kstr.get_bytes("utf-8");
    std::cout<<str<<"\n";

    Tokenize::TermVector tv;
    (*tokenizer_)(userQuery, tv);

    FreqStringVector candidates;
    indexer_->search(tv, candidates);
    StringUtil::removeItem(candidates, userQuery);
    //std::sort(candidates.begin(), candidates.end());
    FreqStringVector uqByFreq;
    for (std::size_t i = 0; (i < candidates.size()) && (i < N); i++)
    {
        std::cout<<candidates[i].getString()<<" : "<<candidates[i].getFreq()<<"\n";
        uqByFreq.push_back(candidates[i]);
    }

    StringUtil::tuneByEditDistance(uqByFreq, userQuery);
    for (std::size_t i = 0; i < uqByFreq.size(); i++)
    {
        results.push_back(uqByFreq[i].getString());
    }
    uqByFreq.clear();
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

void RecommendEngine::buildEngine(const std::string& path)
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
            Parser* parser = parsers_->load(p);
            Parser::iterator it = parser->begin();
            for (; it != parser->end(); ++it)
            {
                //std::cout<<it->userQuery()<<" : "<<it->freq()<<"\n";
                processQuery(it->userQuery(), it->category(), it->freq());
            }
        }
    }

    //tcTable_->flush();
    //uqcTable_->flush();
    indexer_->flush();
}

void RecommendEngine::processQuery(const std::string& userQuery, const std::string& category, const uint32_t freq)
{
    Tokenize::TermVector tv;
    (*tokenizer_)(userQuery, tv);

    Tokenize::TermVector::iterator it = tv.begin();
    for (; it != tv.end(); ++it)
    {
        //std::cout<<it->term()<<" ";
        //tcTable_->insert(it->term(), category, freq); 
    }
    //std::cout<<" : "<<category<<" : "<<freq<<"\n";

    //uqcTable_->insert(userQuery, category, freq);
    indexer_->insert(tv, userQuery, freq);
}

}
}
