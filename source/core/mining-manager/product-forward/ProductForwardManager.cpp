#include "ProductForwardManager.h"
#include <fstream>
#include <boost/filesystem.hpp>
#include <glog/logging.h>
#include "am/util/line_reader.h"
#include <algorithm>
namespace sf1r
{

ProductForwardManager::ProductForwardManager(
        const std::string& dirPath,
        const std::string& propName,
        bool isDebug)
    : dirPath_(dirPath)
    , propName_(propName)
    , lastDocid_(0)
    , isDebug_(isDebug)
{
}

bool ProductForwardManager::open()
{
    return load();   
}

bool ProductForwardManager::save(unsigned int last_doc)
{
    std::string documentScorePath = dirPath_ + "/forward.dict";
    std::string documentNumPath = dirPath_ + "/forward.size";
    fstream fout;
    fstream fout_size;
    fout.open(documentScorePath.c_str(), ios::app | ios::out);
    fout_size.open(documentNumPath.c_str(), ios::out);

    if(fout_size.is_open())
    {
        fout_size << last_doc;
        fout_size.close();
    }
    if(fout.is_open())
    {
        ReadLock lock(mutex_);
        for (unsigned int i = lastDocid_ + 1; i < forward_index_.size(); ++i)
            fout << forward_index_[i] << endl;
        fout.close();
        lastDocid_ = last_doc;
    }
    return true;
}

bool ProductForwardManager::load()
{
    std::string documentScorePath = dirPath_ + "/forward.dict";
    std::string documentNumPath = dirPath_ + "/forward.size";
    fstream fin;
    fstream fin_size;
    if (!boost::filesystem::exists(documentScorePath) || !boost::filesystem::exists(documentNumPath))
    {
        return false;
    }
    fin.open(documentScorePath.c_str(), ios::in);
    fin_size.open(documentNumPath.c_str(), ios::in);

    if (fin_size.is_open())
    {
        fin_size >> lastDocid_;
        fin_size.close();
    }

    std::vector<std::string> tmp_index;
    tmp_index.reserve(lastDocid_ + 1);
    tmp_index.push_back(std::string(""));
    if (fin.is_open())
    {
        char st[45678];
        while (fin.getline(st, 32768, '\n'))
        {
//            if (strlen(st))
            {
                tmp_index.push_back(std::string(st));
            }
        }
        fin.close();
    }
    if (tmp_index.size() != lastDocid_ + 1)
        return false;
    WriteLock lockK(mutex_);
    tmp_index.swap(forward_index_);

    return true;
}

void ProductForwardManager::clear()
{
    {
        WriteLock lock(mutex_);
        std::vector<std::string>().swap(forward_index_);
    }
    std::string documentScorePath = dirPath_ + "/forward.dict";
    std::string documentNumPath = dirPath_ + "/forward.size";
    try
    {
        if (boost::filesystem::exists(documentScorePath))
            boost::filesystem::remove_all(documentScorePath);
        
        if (boost::filesystem::exists(documentNumPath))
            boost::filesystem::remove_all(documentNumPath);   
    }
    catch (std::exception& ex)
    {
        std::cerr<<ex.what()<<std::endl;
    }
}

void ProductForwardManager::resize(unsigned int size)
{
    WriteLock lock(mutex_);
    forward_index_.resize(size);
}

bool ProductForwardManager::insert(std::vector<std::string>& index)
{
    WriteLock lock(mutex_);
    forward_index_.swap(index);
    LOG(INFO)<<"old index size = "<<index.size()<<" new = "<<forward_index_.size();
    return true;
}

void ProductForwardManager::copy(std::vector<std::string>& index)
{
    ReadLock lock(mutex_);
    index = forward_index_;
    if (index.empty())
        index.push_back(std::string(""));
}


std::string ProductForwardManager::getIndex(unsigned int index)
{
    ReadLock lock(mutex_);
    if (index < lastDocid_ && index < forward_index_.size())
        return forward_index_[index];
    return std::string("");
}

bool ProductForwardManager::cmp_(const std::pair<double, docid_t>& x, const std::pair<double, docid_t>& y)
{
    return x.first > y.first;
}

void ProductForwardManager::forwardSearch(const std::string& src, 
  const std::vector<std::pair<double, docid_t> >& docs, 
  std::vector<std::pair<double, docid_t> >& res)
{
    //res = docs;return;
    if (docs.empty() || src.empty())
        return ;
    std::vector<std::pair<double, docid_t> > score;
    score.reserve(docs.size());

    std::vector<uint32_t > q_res;
    uint32_t q_brand, q_model;
    featureParser_.getFeatureIds(src, q_brand, q_model, q_res);
    double q_score = 0;
    for (size_t i = 0; i < q_res.size(); ++i)
        q_score += (i+1)*(i+1);
    for (size_t i = 0; i < docs.size(); ++i)
    {
        double sc = ProductForwardManager::compare_(q_brand, q_model, q_res, q_score, docs[i].second);
        score.push_back(std::make_pair(sc, docs[i].second));
    }
    for (size_t i = 0; i < score.size(); ++i)
        if (score[i].first > 0.8)
            res.push_back(make_pair(docs[i].first+100000*score[i].first, docs[i].second));
    //if (res.size() == 0)res = docs;
}


double ProductForwardManager::compare_(const uint32_t q_brand, const uint32_t q_model, 
  const std::vector<uint32_t>& q_res, const double q_score, const docid_t docid)
{
    const uint32_t MAX_LEN = 5;
    const uint32_t BRAND = 7;
    std::string title_string(getIndex(docid));
    std::vector<uint32_t> t_res;
    uint32_t t_brand = 0;
    uint32_t t_model = 0;
    featureParser_.convertStrToIds(title_string, t_brand, t_model, t_res);

    double same = 0;
    if (q_brand == t_brand && q_brand > 0)
        same  = BRAND*BRAND;
    if (q_model == t_model && q_model > 0)
        same += (BRAND-1)*(BRAND-1);

    if (q_res.empty() || t_res.empty())
        return 0.;

    double q_deno = BRAND*BRAND + (BRAND-1)*(BRAND-1);
    for (size_t i = 0; i < q_res.size(); ++i)
        q_deno += (MAX_LEN-i)*(MAX_LEN-i);
    double t_deno = q_deno;
    for (size_t i = 0; i < t_res.size(); ++i)
        t_deno += (MAX_LEN-i)*(MAX_LEN-i);

    for (size_t i = 0; i < q_res.size(); ++i)
        for (size_t j = 0; j < t_res.size(); ++j)
            if (q_res[i] == t_res[j])
            {   
                same += (MAX_LEN - i) * (MAX_LEN - j); 
                break;
            } 

    return same/sqrt(q_deno*t_deno);
}

}
