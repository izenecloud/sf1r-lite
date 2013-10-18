#include "UQCateEngine.h"
#include <fstream>
#include <boost/filesystem.hpp>
namespace sf1r
{
namespace Recommend
{

static std::string uuid = ".UQCateEngine";

std::string UQCateEngine::workdir;

UQCateEngine::UQCateEngine()
    : owner_(NULL)
{
    if (!boost::filesystem::exists(workdir))
    {
        boost::filesystem::create_directory(workdir);
    }
    
    std::string path = workdir;
    path += "/";
    path += uuid;

    if(!boost::filesystem::exists(path))
        return;
    std::ifstream in;
    in.open(path.c_str(), std::ifstream::in);
    table_.clear();
    in>>*this;
}

UQCateEngine:: ~UQCateEngine()
{
}
    
void UQCateEngine::lock(void* identifier)
{
    if (owner_ != identifier)
        return;
    owner_ = identifier;
}

void UQCateEngine::insert(const std::string& userQuery, const std::string cate, uint32_t freq, const void* identifier)
{
    if (owner_ != identifier)
        return;

    boost::unordered_map<std::string, CateV>::iterator it = table_.find(userQuery);
    if (table_.end() == it)
    {
        CateV cates;
        cates.push_back(std::make_pair(cate, freq));
        table_.insert(std::make_pair(userQuery, cates));
    }
    else
    {
        CateV& cates = it->second;
        std::size_t i = 0;
        for (; i < cates.size(); i++)
        {
            if (cate == cates[i].first)
            {
                cates[i].second += freq;
                break;
            }
        }
        if (i == cates.size())
        {
            cates.push_back(std::make_pair(cate, freq));
        }
    }
}

void UQCateEngine::search(const std::string& userQuery, std::vector<std::pair<std::string, uint32_t> >& results) const
{
    boost::unordered_map<std::string, CateV>::const_iterator it = table_.find(userQuery);
    if (table_.end() == it)
        return;
    results = it->second;
}
    
bool UQCateEngine::cateEqual(const std::string& userQuery, const std::string& v, double threshold) const
{
    boost::unordered_map<std::string, CateV>::const_iterator it = table_.find(userQuery);
    if (table_.end() == it)
        return true;
    const CateV& lCates = it->second;
    
    boost::unordered_map<std::string, uint32_t> cateMap;
    for (std::size_t i = 0; i < lCates.size(); i++)
    {
        if (lCates[i].second > threshold)
            cateMap[lCates[i].first] = lCates[i].second;
    }
    if (cateMap.empty())
        return true;

    it = table_.find(v);
    if (table_.end() == it)
        return false;
    const CateV& rCates = it->second;
    
    for (std::size_t i = 0; i < rCates.size(); i++)
    {
        if (cateMap.find(rCates[i].first) != cateMap.end())
            return true;
    }
    return false;
}

    
void UQCateEngine::flush(const void* identifier) const
{
    if (owner_ != identifier)
        return;
    std::string path = workdir;
    path += "/";
    path += uuid;

    std::ofstream out;
    out.open(path.c_str(), std::ofstream::out | std::ofstream::trunc);
    out<<*this;
}

void UQCateEngine::clear(const void* identifier)
{
    if (owner_ != identifier)
        return;
    table_.clear();
}

std::ostream& operator<<(std::ostream& out, const UQCateEngine& table)
{
    boost::unordered_map<std::string, UQCateEngine::CateV>::const_iterator it = table.table_.begin();
    for (; it != table.table_.end(); it++)
    {
        out<<it->first<<"\t";
        const UQCateEngine::CateV& cates = it->second;
        for (std::size_t i = 0; i < cates.size(); i++)
        {
            out<<cates[i].first<<":"<<cates[i].second<<";";
        }
        out<<"\n";
    }
    return out;
}

std::istream& operator>>(std::istream& in,  UQCateEngine& table)
{
    int maxLine = 10240;
    char cLine[maxLine];
    memset(cLine, 0, maxLine);

    while(in.getline(cLine, maxLine))
    {
        std::string sLine(cLine);
        memset(cLine, 0, maxLine);
        //std::cout<<sLine<<"\n"; 
        std::size_t pos = sLine.find("\t");
        
        if (std::string::npos == pos)
            continue;
        
        if (0 == pos)
            continue;

        std::string userQuery = sLine.substr(0, pos);
        pos += 1;

        UQCateEngine::CateV cates;
        while (true)
        {
            std::size_t found = sLine.find(";", pos);
            if (std::string::npos == found)
                break;
            std::string cLine = sLine.substr(pos, found - pos); // one cate
            pos = found + 1;

            std::size_t seq = cLine.find(":");
            if (std::string::npos == seq)
                continue;

            cates.push_back(std::make_pair(cLine.substr(0, seq), atof(cLine.substr(seq+1).c_str())));
        }
        table.table_.insert(std::make_pair(userQuery, cates));
    }
    return in;
}

}
}
