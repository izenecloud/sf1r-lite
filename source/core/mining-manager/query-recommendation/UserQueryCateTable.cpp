#include "UserQueryCateTable.h"
#include <fstream>
namespace sf1r
{
namespace Recommend
{

std::size_t UserQueryCateTable::N = 32;
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
}

}
}
