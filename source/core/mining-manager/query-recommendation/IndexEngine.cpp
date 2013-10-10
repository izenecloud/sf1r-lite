#include "IndexEngine.h"

#include <stdlib.h>
#include <boost/fusion/sequence/comparison/greater_equal.hpp>
#include <boost/fusion/include/greater_equal.hpp>

namespace sf1r
{
namespace Recommend
{

IndexEngine::IndexEngine(const std::string& dir)
    : db_(NULL)
    , idManager_(NULL)
    , dir_(dir)
{
    open_();
}

IndexEngine::~IndexEngine()
{
    close_();
}

void IndexEngine::open_()
{
    try
    {
        db_ = new MIRDatabase(dir_);
        db_->open();
    }
    catch (std::exception& ex)
    {
        std::cout<<ex.what()<<std::endl;
        return;
    }

    idManager_ = new sf1r::ConceptIDManager(dir_);
    idManager_->Open();
}

void IndexEngine::close_()
{
    if (NULL != db_)
    {
        db_->close();
        delete db_;
        db_ = NULL;
    }

    if (NULL != idManager_)
    {
        idManager_->Close();
        delete idManager_;
        idManager_ = NULL;
    }
}

void IndexEngine::flush()
{
    static uint32_t docId = 1;
    static StringUtil::HashFunc generator;
    
    boost::unordered_map<std::string, std::pair<uint32_t, Tokenize::TermVector> >::iterator it =  userQueries_.begin();
    for (; it != userQueries_.end(); it++)
    {
        MIRDocument doc;
        Tokenize::TermVector::iterator it_tokens = it->second.second.begin();
        for (; it_tokens != it->second.second.end(); it_tokens++)
        {
            izenelib::ir::irdb::IRTerm term;
            term.termId_ = generator(it_tokens->term());
            term.field_ = "default";
            term.position_ = 0;
            doc.addTerm(term);
        }
        
        uint16_t count = 0;
        if (it->second.first > std::numeric_limits<uint16_t>::max())
            count = std::numeric_limits<uint16_t>::max();
        else
            count = it->second.first;
        doc.setData<0>((uint8_t)count);
        doc.setData<1>((uint8_t)(count>>8));
        db_->addDocument(docId, doc);
    
        izenelib::util::UString ustr(it->first, izenelib::util::UString::UTF_8);
        idManager_->Put(docId, ustr);
        docId++;
    }
    
    if (NULL != db_)
    {
        db_->flush();
    }

    if (NULL != idManager_)
    {
        idManager_->Flush();
    }
}

void IndexEngine::insert(Tokenize::TermVector& tv, const std::string& str, uint32_t count)
{

    boost::unordered_map<std::string, std::pair<uint32_t, Tokenize::TermVector> >::iterator it =  userQueries_.find(str);
    if (userQueries_.end() == it)
    {
        userQueries_.insert(std::make_pair(str, std::make_pair(count, tv)));
    }
    else
    {
        it->second.first += count;
    }
}

void IndexEngine::search(Tokenize::TermVector& tv, FreqStringVector& strs)
{
    static StringUtil::HashFunc generator;
    Tokenize::TermVector::iterator it = tv.begin();
    izenelib::am::rde_hash<uint32_t, uint32_t> posMap;
    uint32_t *pos = NULL;
    std::vector<std::pair<uint32_t, uint32_t> > seq;
    typedef izenelib::util::second_greater<std::pair<uint32_t, uint32_t> >greater_than;

    for (; it != tv.end(); it++)
    {
        const std::string term = it->term();
        termid_t termId= generator(term);
        
        try
        {
            std::deque<docid_t> docIdList;
            bool ret = db_->getMatchSet(termId, docIdList);
            if (ret)
            {
                for (std::size_t ii = 0; ii < docIdList.size(); ii++)
                {
                    uint32_t count = 0;
                    uint8_t low = 0;
                    uint8_t high = 0;
                    db_->getDocData<0>(docIdList[ii], low);
                    db_->getDocData<1>(docIdList[ii], high);
                    count = high;
                    count = count << 8;
                    count += low;
                    
                    if (NULL == (pos = posMap.find(docIdList[ii])))
                    {
                        posMap.insert(docIdList[ii], seq.size());
                        seq.push_back(std::make_pair(docIdList[ii], count));
                    }
                    else
                    {
                        seq[*pos].second += count;
                    }
                }
            }
        }
        catch (std::exception& ex)
        {
            std::cout<<ex.what()<<"\n";
            return;
        }
    }
    std::sort(seq.begin(), seq.end(), greater_than());
    for (std::size_t i = 0; i < seq.size(); i++)
    {
        izenelib::util::UString ustr;
        idManager_->GetStringById(seq[i].first, ustr);
        std::string str;
        ustr.convertString(str, izenelib::util::UString::UTF_8);
                     
        FreqString item(str, seq[i].second);
        strs.push_back(item);
    }
    //std::sort(strs.begin(), strs.end());
    //std::reverse(strs.begin(), strs.end());
    //StringUtil::removeDuplicate(strs);
}

}
}
