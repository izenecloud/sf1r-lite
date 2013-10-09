#include "TermCateTable.h"
#include "StringUtil.h"

#include <utility>
#include <boost/filesystem.hpp>
#include <fstream>

namespace sf1r
{
namespace Recommend
{

static std::string uuid = ".TermCateTable";

TermCateTable::TermCateTable(const std::string& workdir)
    : workdir_(workdir)
{
    if(!boost::filesystem::exists(workdir_))
    {
        boost::filesystem::create_directory(workdir_);
    }
    tcTable_.clear();

    std::string path = workdir_;
    path += "/";
    path += uuid;

    std::ifstream in;
    in.open(path.c_str(), std::ifstream::in);
    in>>*this;
}

TermCateTable::~TermCateTable()
{
    tcTable_.clear();
}

void TermCateTable::insert(const std::string& term, const std::string& cate, const uint32_t freq)
{
    CateId id = StringUtil::strToInt(cate, idmap_);
    boost::unordered_map<std::string, CateIdList>::iterator it = tcTable_.find(term);
    if (tcTable_.end() == it) // new term
    {
        CateIdList ids;
        Cate c(id, freq);
        ids.push_back(c);
        tcTable_.insert(std::make_pair(term, ids));
    }
    else // update
    {
        
        CateIdList& ids = it->second;
        CateIdList::iterator it = ids.begin();
        for (; it != ids.end(); it++)
        {
            if (id == it->first)
            {
                it->second += freq;
                break;
            }
        }
        if (it == ids.end()) // new category
        {
            ids.push_back(Cate(id, freq));
        }
    }
}

void TermCateTable::category(const std::string& term, CateIdList& cates)
{
    boost::unordered_map<std::string, CateIdList>::iterator it = tcTable_.find(term);
    if (tcTable_.end() == it)
    {
    }
    else
    {
        cates = it->second;
    }
}

void TermCateTable::flush()
{
    std::string path = workdir_;
    path += "/";
    path += uuid;

    std::ofstream out;
    out.open(path.c_str(), std::ofstream::out | std::ofstream::trunc);
    out<<*this;
}

std::string TermCateTable::cateIdToCate(CateId id)
{
    return StringUtil::intToStr(id, idmap_);
}

std::ostream& operator<<(std::ostream& out, const TermCateTable& tc)
{
    boost::unordered_map<std::string, TermCateTable::CateIdList>::const_iterator it = tc.tcTable_.begin();
    for (; it != tc.tcTable_.end(); it++)
    {
        out<<it->first<<"::";
        const TermCateTable::CateIdList& ids = it->second;
        TermCateTable::CateIdList::const_iterator iit = ids.begin();
        for (; iit != ids.end(); iit++)
        {
            out<<iit->first<<":"<<iit->second<<";";
        }
        out<<"\n";
    }
    out<<"---idmap----\n";
    boost::bimap<int, std::string>::left_const_iterator bit;
    for (bit = tc.idmap_.left.begin(); bit != tc.idmap_.left.end(); bit++)
    {
        out<<bit->first<<":"<<bit->second<<"\n";
    }
    
    return out;
}

std::istream& operator>>(std::istream& in,  TermCateTable& tc)
{
    int maxLine = 102400;
    char cLine[maxLine];
    memset(cLine, 0, maxLine);
    bool isIdmap = false;

    while(in.getline(cLine, maxLine))
    {
        std::string sLine(cLine);
        memset(cLine, 0, maxLine);
        //std::cout<<sLine<<"\n";
        if ("---idmap----" == sLine)
        {
            isIdmap = true;
            continue;
        }
        if (isIdmap)
        {
            std::size_t pos = sLine.find(":");
            if (std::string::npos == pos)
                continue;
            int id = atoi(sLine.substr(0, pos).c_str());
            std::string cate = sLine.substr(pos+1);
            //std::cout<<id<<":"<<cate<<"\n";
            tc.idmap_.insert(boost::bimap<int, std::string>::value_type(id, cate));
            continue;
        }
        std::size_t pos = sLine.find("::");
        if (std::string::npos == pos)
            continue;
        
        std::string term = sLine.substr(0, pos);
        pos += 2;
        TermCateTable::CateIdList ids;
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
            ids.push_back(TermCateTable::Cate(atoi(pair.substr(0, seq).c_str()), atoi(pair.substr(seq+1).c_str())));
        }
        tc.tcTable_.insert(std::make_pair(term, ids));
    }
    
    return in;
}

TermCateTable::CateIdList& operator+=(TermCateTable::CateIdList& lv, const TermCateTable::CateIdList& rv)
{
    TermCateTable::CateIdList::const_iterator it = rv.begin();
    for (; it != rv.end(); it++)
    {
        TermCateTable::CateIdList::iterator it_self = lv.begin();
        for (; it_self != lv.end(); it_self++)
        {
            if (it->first == it_self->first)
                break;
        }
        if (it_self == lv.end())
            lv.push_back(TermCateTable::Cate(it->first, it->second));
        else
            it_self->second += it->second;
    }
    return lv;
}

bool operator<(const TermCateTable::Cate& lv, const TermCateTable::Cate& rv)
{
    return lv.second < rv.second;
}

std::ostream& operator<<(std::ostream& out, const TermCateTable::CateIdList& v)
{
    TermCateTable::CateIdList::const_iterator it = v.begin();
    for (; it != v.end(); it++)
    {
        out<<it->first<<":"<<it->second<<" ";
    }
    out<<"\n";
    return out;
}
}
}
