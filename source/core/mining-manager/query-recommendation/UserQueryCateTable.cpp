#include "UserQueryCateTable.h"
#include <fstream>
#include <boost/filesystem.hpp>
namespace sf1r
{
namespace Recommend
{

static std::string uuid = ".UserQueryCateTable";

UserQueryCateTable::UserQueryCateTable(const std::string& workdir)
    : workdir_(workdir)
{
    std::string path = workdir_;
    path += "/";
    path += uuid;

    if(!boost::filesystem::exists(path))
        return;
    std::ifstream in;
    in.open(path.c_str(), std::ifstream::in);
    table_.clear();
    in>>*this;
}

UserQueryCateTable:: ~UserQueryCateTable()
{
}
    
void UserQueryCateTable::insert(const std::string& userQuery, const std::string cate, uint32_t freq)
{
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

void UserQueryCateTable::search(const std::string& userQuery, std::vector<std::pair<std::string, uint32_t> >& results) const
{
    boost::unordered_map<std::string, CateV>::const_iterator it = table_.find(userQuery);
    if (table_.end() == it)
        return;
    results = it->second;
}
    
bool UserQueryCateTable::cateEqual(const std::string& lv, const std::string& rv) const
{
    boost::unordered_map<std::string, CateV>::const_iterator it = table_.find(lv);
    if (table_.end() == it)
        return false;
    const CateV& lCates = it->second;
    
    //CateT maxLv;
    //uint32_t max = 0;
    boost::unordered_map<std::string, uint32_t> cateMap;
    uint32_t threshold = 500;
    for (std::size_t i = 0; i < lCates.size(); i++)
    {
        //if (lCates[i].second > max)
        //{
        //    max = lCates[i].second;
        //    maxLv = lCates[i];
        //}
        if (lCates[i].second > threshold)
            cateMap[lCates[i].first] = lCates[i].second;
    }

    /*const std::string sLCate = maxLv.first;
    uint8_t n = 0;
    std::string strL;
    for (std::size_t i = 0; i < sLCate.size(); i++)
    {
        if (sLCate[i] == '>')
        {
            if (++n > 2)
                break;
        }
        strL += sLCate[i];
    }*/

    it = table_.find(rv);
    if (table_.end() == it)
        return false;
    const CateV& rCates = it->second;
    
    //CateT maxRv;
    //max = 0;
    for (std::size_t i = 0; i < rCates.size(); i++)
    {
        if (cateMap.find(rCates[i].first) != cateMap.end())
            return true;
    //    if (rCates[i].second > max)
    //    {
    //        max = rCates[i].second;
    //        maxRv = rCates[i];
    //    }
    }
    return false;
    
    /*
    const std::string sRCate = maxLv.first;
    n = 0;
    std::string strR;
    for (std::size_t i = 0; i < sRCate.size(); i++)
    {
        if (sRCate[i] == '>')
        {
            if (++n > 2)
                break;
        }
        strR += sRCate[i];
    }
    
    return strL == strR;
    */
}

    
void UserQueryCateTable::flush() const
{
    std::string path = workdir_;
    path += "/";
    path += uuid;

    std::ofstream out;
    out.open(path.c_str(), std::ofstream::out | std::ofstream::trunc);
    out<<*this;
}

void UserQueryCateTable::clear()
{
    table_.clear();
}

std::ostream& operator<<(std::ostream& out, const UserQueryCateTable& table)
{
    boost::unordered_map<std::string, UserQueryCateTable::CateV>::const_iterator it = table.table_.begin();
    for (; it != table.table_.end(); it++)
    {
        out<<it->first<<"\t";
        const UserQueryCateTable::CateV& cates = it->second;
        for (std::size_t i = 0; i < cates.size(); i++)
        {
            out<<cates[i].first<<":"<<cates[i].second<<";";
        }
        out<<"\n";
    }
    return out;
}

std::istream& operator>>(std::istream& in,  UserQueryCateTable& table)
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

        UserQueryCateTable::CateV cates;
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
/*std::size_t UserQueryCateTable::N = 32;
static std::string uuid = ".UserQueryCateTable";

UserQueryCateTable::UserQueryCateTable(const std::string& workdir)
    : workdir_(workdir)
{
    std::string path = workdir_;
    path += "/";
    path += uuid;

    std::ifstream in;
    in.open(path.c_str(), std::ofstream::in);    
    in>>*this;
}

UserQueryCateTable:: ~UserQueryCateTable()
{
}

void UserQueryCateTable::insert(const std::string& userQuery, const std::string& cate, uint32_t freq)
{
    boost::unordered_map<std::string, TopNQueue>::iterator it = topNUserQuery_.find(cate);
    if (topNUserQuery_.end() == it)
    {
        UserQuery uq(freq, userQuery);
        TopNQueue uqq;
        uqq.push(uq);
        topNUserQuery_.insert(std::make_pair(cate, uqq));
        return;
    }

    UserQuery uq(freq, userQuery);
    if (it->second.size() < N)
        it->second.push(uq);
    else if (it->second.top().freq() < freq)
    {
        it->second.pop();
        it->second.push(uq);
    }
}

void UserQueryCateTable::topNUserQuery(const std::string& cate, const uint32_t N, UserQueryList& userQuery) const
{
    boost::unordered_map<std::string, TopNQueue>::const_iterator it = topNUserQuery_.find(cate);
    if (topNUserQuery_.end() == it)
        return;
    TopNQueue uqq = it->second;
    uint32_t n = 0;
    while (!uqq.empty())
    {
        userQuery.push_back(uqq.top());
        uqq.pop();
        if (++n >= N)
            break;
    }
}
    
void UserQueryCateTable::flush()
{
    std::string path = workdir_;
    path += "/";
    path += uuid;

    std::ofstream out;
    out.open(path.c_str(), std::ofstream::out | std::ofstream::trunc);
    out<<*this;
}

std::ostream& operator<<(std::ostream& out, const UserQueryCateTable& uqc)
{
    boost::unordered_map<std::string, UserQueryCateTable::TopNQueue>::const_iterator it = uqc.topNUserQuery_.begin();
    for (; it != uqc.topNUserQuery_.end(); it++)
    {
        out<<it->first<<"::";
        UserQueryCateTable::TopNQueue uqq = it->second;
        while (!uqq.empty())
        {
            out<<uqq.top().userQuery()<<":"<<uqq.top().freq()<<";";
            uqq.pop();
        }
        out<<"\n";
    }
    return out;
}

std::istream& operator>>(std::istream& in,  UserQueryCateTable& uqc)
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
        
        std::string cate = sLine.substr(0, pos);
        pos += 2;
        UserQueryCateTable::TopNQueue userQuery;
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
            userQuery.push(UserQueryCateTable::UserQuery(atoi(pair.substr(seq+1).c_str()), pair.substr(0, seq)));
            //std::cout<<atoi(pair.substr(0, seq).c_str())<<" : "<<pair.substr(seq+1).c_str()<<"\n";
        }
        uqc.topNUserQuery_.insert(std::make_pair(cate, userQuery));
    }
    return in;
}

bool operator<(const UserQueryCateTable::UserQuery& lv, const UserQueryCateTable::UserQuery& rv)
{
    return lv.freq() < rv.freq();
}*/

}
}
