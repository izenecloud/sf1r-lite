#include "PinyinTable.h"
#include <fstream>
#include <boost/filesystem.hpp>

namespace sf1r
{
namespace Recommend
{

static std::string uuid = "PinyinTable";
PinyinTable::PinyinTable(const std::string& workdir)
    : workdir_(workdir)
{
    std::string path = workdir_;
    path += "/";
    path += uuid;

    if(!boost::filesystem::exists(path))
        return;
    std::ifstream in;
    in.open(path.c_str(), std::ifstream::in);
    pinyinTable_.clear();
    in>>*this;
    //std::cout<<"PinyinTable::"<<pinyinTable_.size()<<"\n";
}

PinyinTable::~PinyinTable()
{
}

void PinyinTable::insert(const std::string& userQuery, const std::string& pinyin, uint32_t freq)
{
    UserQuery uq(userQuery, freq);
    boost::unordered_map<std::string, UserQueryList>::iterator it = pinyinTable_.find(pinyin);
    if (pinyinTable_.end() != it)
    { 
        UserQueryList::iterator uq_it =it->second.begin();
        for (; uq_it != it->second.end(); uq_it++)
        {
            if (uq_it->userQuery() == uq.userQuery())
                break;
        }
        if (uq_it != it->second.end())
            uq_it->setFreq(uq_it->freq() + freq);
        else
            it->second.push_back(uq);
        return;
    }
    UserQueryList uqList;
    uqList.push_back(uq);
    pinyinTable_.insert(make_pair(pinyin, uqList));

}

void PinyinTable::search(const std::string& pinyin, UserQueryList& uqlist) const
{
    boost::unordered_map<std::string, UserQueryList>::const_iterator it = pinyinTable_.find(pinyin);
    if (pinyinTable_.end() != it)
        uqlist = it->second;
}
    
void PinyinTable::clear()
{
    pinyinTable_.clear();
}

void PinyinTable::flush() const
{
    std::string path = workdir_;
    path += "/";
    path += uuid;

    std::ofstream out;
    out.open(path.c_str(), std::ofstream::out | std::ofstream::trunc);
    out<<*this;
    //std::cout<<"PinyinTable::"<<pinyinTable_.size()<<"\n";
}
    
std::ostream& operator<<(std::ostream& out, const PinyinTable& table)
{
    boost::unordered_map<std::string, UserQueryList>::const_iterator it = table.pinyinTable_.begin();
    for (; it != table.pinyinTable_.end(); it++)
    {
        out<<it->first<<"::";
        UserQueryList::const_iterator uq_it = it->second.begin();
        for (; uq_it != it->second.end(); uq_it++)
        {
            out<<uq_it->userQuery()<<":"<<uq_it->freq()<<";";
        }
        out<<"\n";
    }
    return out;
}

std::istream& operator>>(std::istream& in,  PinyinTable& table)
{
    int maxLine = 102400;
    char cLine[maxLine];
    memset(cLine, 0, maxLine);

    while(in.getline(cLine, maxLine))
    {
        std::string sLine(cLine);
        memset(cLine, 0, maxLine);
        
        std::size_t pos = sLine.find("::");
        if (std::string::npos == pos)
            continue;
        
        std::string pinyin = sLine.substr(0, pos);
        pos += 2;
        UserQueryList uqList;
        while (true)
        {
            std::size_t found = sLine.find(";", pos);
            if (std::string::npos == found)
                break;
            
            std::string pair = sLine.substr(pos, found - pos);
            pos = found + 1;

            std::size_t seq = pair.find(":");
            if (std::string::npos == seq)
                continue;
            uqList.push_back(UserQuery(pair.substr(0, seq), atoi(pair.substr(seq+1).c_str())));
        }
        table.pinyinTable_.insert(std::make_pair(pinyin, uqList));
    }
    return in;
}

}
}
