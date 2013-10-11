#include "IndexEngine.h"

#include <stdlib.h>
#include <boost/fusion/sequence/comparison/greater_equal.hpp>
#include <boost/fusion/include/greater_equal.hpp>

namespace sf1r
{
namespace Recommend
{

//static std::string uuid = ".Determiner";
IndexEngine::IndexEngine(const std::string& dir)
    : db_(NULL)
    , idManager_(NULL)
    , bf_(NULL)
    , tokenizer_(NULL)
    , dir_(dir)
{
    open_();
    
    bf_ = new BloomFilter(32, 1e-8, 32);
    //std::string path = dir_;
    //path += "/";
    //path += uuid;

    //if(boost::filesystem::exists(path))
    //{
    //    std::ifstream in;
    //    in.open(path.c_str(), std::ifstream::in);
    //    bf_->load(in);
    //}
    //else
    {
        bf_->Insert("新款");
        bf_->Insert("正品");
        bf_->Insert("包邮");
        bf_->Insert("秒杀");
        bf_->Insert("清仓");
        bf_->Insert("特级");
        bf_->Insert("加厚");
        bf_->Insert("特大");
        bf_->Insert("男");
        bf_->Insert("男式");
        bf_->Insert("女");
        bf_->Insert("女式");
        bf_->Insert("2011");
        bf_->Insert("2012");
        bf_->Insert("2013");
    }
}

IndexEngine::~IndexEngine()
{
    if (NULL != bf_)
    {
        delete bf_;
        bf_ = NULL;
    }
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

void IndexEngine::clear()
{

}

void IndexEngine::flush()
{
    static uint32_t docId = 1;
    static StringUtil::HashFunc generator;
   
    if (tokenizer_ == NULL)
        return;

    boost::unordered_map<std::string, uint32_t>::iterator it =  userQueries_.begin();
    for (; it != userQueries_.end(); it++)
    {
        Tokenize::TermVector tv;
        (*tokenizer_)(it->first, tv);

        MIRDocument doc;
        Tokenize::TermVector::iterator it_tokens = tv.begin();
        for (; it_tokens != tv.end(); it_tokens++)
        {
            izenelib::ir::irdb::IRTerm term;
            term.termId_ = generator(it_tokens->term());
            term.field_ = "default";
            term.position_ = 0;
            doc.addTerm(term);
        }
        
        uint16_t count = 0;
        if (it->second > std::numeric_limits<uint16_t>::max())
            count = std::numeric_limits<uint16_t>::max();
        else
            count = it->second;
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
    
    /*std::string path = dir_;
    path += "/";
    path += uuid;
    
    std::ofstream out;
    out.open(path.c_str(), std::ofstream::out | std::ofstream::trunc);
    bf_->save(out);*/
}

void IndexEngine::insert(const std::string& userQuery, uint32_t count)
{
    /*std::string nstr = "";
    for (std::size_t i = 0; i < str.size(); i++)
    {
        if (!isspace(str[i])
            nstr += str[i];
    }*/

    boost::unordered_map<std::string, uint32_t>::iterator it =  userQueries_.find(userQuery);
    if (userQueries_.end() == it)
    {
        userQueries_.insert(std::make_pair(userQuery, count));
    }
    else
    {
        it->second += count;
    }
}

void IndexEngine::search(const std::string& userQuery, FreqStringVector& byCate, FreqStringVector& byFreq, uint32_t N, const CateEqualer* equaler) const
{
    static StringUtil::HashFunc generator;
   
    if (tokenizer_ == NULL)
        return;
    
    /*std::string query = userQuery;

    Tokenize::TermVector tv;
    izenelib::util::UString uQuery(query, izenelib::util::UString::UTF_8);
    static izenelib::util::UString MAN("男", izenelib::util::UString::UTF_8);
    static izenelib::util::UString WOMAN("女", izenelib::util::UString::UTF_8);
    izenelib::util::UString uq;
    for (std::size_t i = 0; i < uQuery.length(); i++)
    {
        {
            if (MAN[0] == uQuery[i] || WOMAN[0] == uQuery[i])
            {
                izenelib::util::UString uTerm;
                uTerm += uQuery[i];
                Tokenize::Term item(uTerm, 0.0);
                tv.push_back(item);
            }
            else
            {
                uq += uQuery[i];
            }
        }
    }
    if (uq.length() > 1)
    {
        query.clear();
        uq.convertString(query, izenelib::util::UString::UTF_8);
    }
    else
    {
        tv.clear();
    }
    
    (*tokenizer_)(query, tv);*/
    Tokenize::TermVector tv;
    (*tokenizer_)(userQuery, tv);
    
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
                        if ((bf_->Get(term)) && (tv.size() > 1))
                            continue;
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

    boost::unordered_map<std::string, std::size_t> pFreq;
    boost::unordered_map<std::string, std::size_t> pCate;
    uint32_t n = 0;
    for (std::size_t i = 0; i < seq.size(); i++)
    {
        izenelib::util::UString ustr;
        idManager_->GetStringById(seq[i].first, ustr);
        std::string str;
        ustr.convertString(str, izenelib::util::UString::UTF_8);
        
        if (StringUtil::isNeedRemove(str, userQuery))
            continue;
        
        std::string rstr; // string removed space
        StringUtil::removeSpace(str, rstr);
        std::string srstr(rstr); // sorted removed space string
        std::sort(srstr.begin(), srstr.end());
        
        if((*equaler)(userQuery, str))
        {
            boost::unordered_map<std::string, std::size_t>::iterator pit = pCate.find(srstr);
            if (pCate.end() == pit)
            {
                pCate.insert(std::make_pair(srstr, byCate.size()));
                FreqString item(rstr, seq[i].second);
                byCate.push_back(item);
                if (++n >= N)
                    break;
            }
            else
            {
                byCate[pit->second].setFreq(byCate[pit->second].getFreq() + seq[i].second);
            }
        }
        else 
        {
            if (byFreq.size() > N)
                continue;
            boost::unordered_map<std::string, std::size_t>::iterator pit = pFreq.find(srstr);
            if (pFreq.end() == pit)
            {
                pFreq.insert(std::make_pair(srstr, byFreq.size()));
                FreqString item(rstr, seq[i].second);
                byFreq.push_back(item);
            }
            else
            {
                byFreq[pit->second].setFreq(byFreq[pit->second].getFreq() + seq[i].second);
            }
        }
    }
}

}
}
