#include "PrefixTable.h"
#include "StringUtil.h"
#include <fstream>
#include <boost/filesystem.hpp>
#include <limits>
#include <util/ustring/UString.h>

namespace sf1r
{
namespace Recommend
{

static std::string uuid = ".PrefixTable";
std::size_t PrefixTable::PREFIX_SIZE = 2;

PrefixTable::PrefixTable(const std::string& workdir)
    : workdir_(workdir)
{
    std::string path = workdir_;
    path += "/";
    path += uuid;

    if(!boost::filesystem::exists(path))
        return;
    std::ifstream in;
    in.open(path.c_str(), std::ifstream::in);
    in>>*this;
    //std::cout<<"PrefixTable::"<<table_.size()<<"\n";
}

PrefixTable::~PrefixTable()
{
}

void PrefixTable::prefix(const izenelib::util::UString& userQuery, izenelib::util::UString& pref)
{
    //izenelib::util::UString ustr(userQuery, izenelib::util::UString::UTF_8);
    //izenelib::util::UString uPref;
    userQuery.substr(pref, 0, PREFIX_SIZE);
    //uPref.convertString(pref, izenelib::util::UString::UTF_8);
    /*
    const char* pos = userQuery.c_str();
    const char* end = pos + userQuery.size();
    std::size_t n = 0;
    bool isLastChar = false;
    for (; pos < end; pos++)
    {
        if (isalpha(*pos) || isdigit(*pos))
        {
            if (!isLastChar)
            {
                isLastChar = true;
            }
            pref += *pos;
        }
        else if (isblank(*pos)) // end of english word
        {
            pref += *pos;
            if (isLastChar)
                n++;
        }
        else //chinese character
        {
            if (isLastChar) // end of english word;
            {
                n++;
            }
            if (n > PREFIX_SIZE)
                break;
            if (end - pos >= 3)
            {
                pref += std::string(pos, 3);
                pos += 3;
            }
            n++;
        }
        if (n > PREFIX_SIZE)
            break;
    }*/
}

bool PrefixTable::isPrefixEnglish(const std::string& userQuery)
{
    izenelib::util::UString ustr(userQuery, izenelib::util::UString::UTF_8);
    izenelib::util::UString uPrefix;
    PrefixTable::prefix(ustr,uPrefix);
    
    std::string prefix;
    uPrefix.convertString(prefix, izenelib::util::UString::UTF_8);
    return StringUtil::isEnglish(prefix);
}

void PrefixTable::insert(const std::string& userQuery, uint32_t freq)
{
    izenelib::util::UString ustr(userQuery, izenelib::util::UString::UTF_8);
    std::size_t size = ustr.length();
    
    izenelib::util::UString uPrefix;
    PrefixTable::prefix(ustr,uPrefix);
    size -= uPrefix.length();
   
    std::string prefix;
    uPrefix.convertString(prefix, izenelib::util::UString::UTF_8);
    
    UserQuery uq(userQuery, freq);
    boost::unordered_map<std::string, LeveledUQL>::iterator it = table_.find(prefix);
    if (table_.end() == it) // new item
    {
        LeveledUQL luqList(size + 1);
        luqList[size].push_back(uq);
        table_.insert(make_pair(prefix, luqList));
        return;
    }
    
    LeveledUQL& luqList = it->second;
    if (size >= luqList.size()) // new size level
    {
        luqList.resize(size + 1);
        luqList[size].push_back(uq);
    }
    else
    {
        UserQueryList& uqList = luqList[size];
        UserQueryList::iterator it = uqList.begin();
        for (; it != uqList.end(); it++)
        {
            if (it->userQuery() == uq.userQuery())
                break;
        }
        if (it != uqList.end())
            it->setFreq(it->freq() + freq);
        else
            luqList[size].push_back(uq);
    }
}

void PrefixTable::search(const std::string& userQuery, UserQueryList& results) const
{
    izenelib::util::UString ustr(userQuery, izenelib::util::UString::UTF_8);
    std::size_t size = ustr.length();
    
    izenelib::util::UString uPrefix;
    PrefixTable::prefix(ustr,uPrefix);
    size -= uPrefix.length();
    std::string prefix;
    uPrefix.convertString(prefix, izenelib::util::UString::UTF_8);
    
    boost::unordered_map<std::string, LeveledUQL>::const_iterator it = table_.find(prefix);
    if (table_.end() == it)
        return;
    const LeveledUQL& luqList = it->second;
    const std::size_t lsize = luqList.size();
    if (luqList.empty() || ((size > 0) && (lsize < size - 1)))
        return;
    if ((size > 1) && (lsize > size -1))
    {
        const UserQueryList& uqList = luqList[size - 1];
        UserQueryList::const_iterator it = uqList.begin();
        for (; it != uqList.end(); it++)
        {
            int d = StringUtil::editDistance(it->userQuery(), userQuery);
            if (d > 1)
                continue;
            results.push_back(*it);
        }

    }
    if (lsize > size)
    {
        const UserQueryList& uqList = luqList[size];
        UserQueryList::const_iterator it = uqList.begin();
        for (; it != uqList.end(); it++)
        {
            int d = StringUtil::editDistance(it->userQuery(), userQuery);
            if (d > 1)
                continue;
            results.push_back(*it);
        }
    }
    if (lsize > size + 1)
    {
        const UserQueryList& uqList = luqList[size + 1];
        UserQueryList::const_iterator it = uqList.begin();
        for (; it != uqList.end(); it++)
        {
            int d = StringUtil::editDistance(it->userQuery(), userQuery);
            if (d > 1)
                continue;
            results.push_back(*it);
        }
    }
}

void PrefixTable::clear()
{
    table_.clear();
}

void PrefixTable::flush() const
{
    std::string path = workdir_;
    path += "/";
    path += uuid;

    std::ofstream out;
    out.open(path.c_str(), std::ofstream::out | std::ofstream::trunc);
    out<<*this;
    //std::cout<<"PinyinTable::"<<table_.size()<<"\n";
}

std::ostream& operator<<(std::ostream& out, const PrefixTable& table)
{
    boost::unordered_map<std::string, PrefixTable::LeveledUQL>::const_iterator it = table.table_.begin();
    for(; it != table.table_.end(); it++)
    {
        out<<it->first<<"::";
        PrefixTable::LeveledUQL::const_iterator luq_it = it->second.begin();
        int level = 0;
        for (; luq_it != it->second.end(); luq_it++, level++)
        {
            out<<level<<">";
            UserQueryList::const_iterator uq_it = luq_it->begin();
            for (; uq_it != luq_it->end(); uq_it++)
            {
                out<<uq_it->userQuery()<<":"<<uq_it->freq()<<";";
            }
            out<<"--"; // level
        }
        out<<"\n";
    }
    return out;
}

std::istream& operator>>(std::istream& in,  PrefixTable& table)
{
    int maxLine = 1024000;
    char cLine[maxLine];
    memset(cLine, 0, maxLine);

    while(in.getline(cLine, maxLine))
    {
        std::string sLine(cLine);
        memset(cLine, 0, maxLine);
        //std::cout<<sLine<<"\n"; 
        std::size_t pos = sLine.find("::");
        if (std::string::npos == pos)
            continue;
        
        if (0 == pos)
            continue;

        std::string prefix = sLine.substr(0, pos);
        pos += 2;

        PrefixTable::LeveledUQL luqList;
        while (true)
        {
            std::size_t found = sLine.find("--", pos);
            if (std::string::npos == found)
                break;

            
            UserQueryList uqList;
            std::size_t level = std::numeric_limits<std::size_t>::max();
            std::string sLevel = sLine.substr(pos, found - pos);
            
            pos = found + 2;
            
            std::size_t lpos = 0; // level pos
            found = sLevel.find(">");
            if (std::string::npos == found)
                break;
            level = atoi(sLevel.substr(0, found).c_str());
            //std::cout<<level<<"\t"<<sLevel<<"\n";
            lpos = found +1;
            while (true)
            {
                std::size_t lfound = sLevel.find(";", lpos);
                if (std::string::npos == lfound)
                    break;
                std::string pair = sLevel.substr(lpos, lfound - lpos);
                lpos = lfound + 1;

                std::size_t seq = pair.find(":");
                if (std::string::npos == seq)
                    continue;
                
                //std::cout<<pair.substr(0, seq)<<" : "<<atoi(pair.substr(seq+1).c_str())<<"\n";
                uqList.push_back(UserQuery(pair.substr(0, seq).c_str(), atoi(pair.substr(seq+1).c_str())));
            }
            if (std::numeric_limits<std::size_t>::max() == level)
                continue;
            if (luqList.size() <= level)
                luqList.resize(level + 1);
            if (!uqList.empty())
                luqList[level] = uqList;
            //std::cout<<level<<" : "<<luqList[level].size()<<"\n";
            
        }
        //std::cout<<prefix<<"\n";
        table.table_.insert(std::make_pair(prefix, luqList));
    }
    return in;
}
    
}
}
