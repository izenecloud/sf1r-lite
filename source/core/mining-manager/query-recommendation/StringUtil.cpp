#include "StringUtil.h"

#include <iostream>
#include <util/ustring/UString.h>

namespace sf1r
{
namespace Recommend
{
namespace StringUtil
{
std::string& normalize(std::string& str)
{
    boost::trim(str);
    const char* pos = str.c_str();
    std::size_t size = str.size();
    std::size_t lastPos = 0;
    std::string nstr = "";
    for (std::size_t i = 0; i < size; i++)
    {
        if (' ' == pos[i])
        {
            nstr += std::string(pos + lastPos, i - lastPos);
            nstr += " ";
            for (; i < size; i++)
            {
                if ((i + 1 < size) && (pos[i+1] != ' '))
                    break;
            }
            lastPos = i + 1;
        }
    }
    nstr += std::string(pos + lastPos);
    str = nstr;
    boost::trim(str);
    boost::to_lower(str);
    return str;
}

void removeDuplicate(StringVector& strs)
{
    if ( 1 >= strs.size())
        return;
    StringVector sstrs = strs; //sorted strings
    std::sort(sstrs.begin(), sstrs.end());
    std::string rstr = "";
    bool isReplicate = false;
    for (std::size_t i = 0; i < sstrs.size() - 1; i++)
    {
        if(sstrs[i] == sstrs[i+1])
        {
            rstr = sstrs[i];
            isReplicate = true;
        }
        else
        {
            if (isReplicate)
            {
                bool isFirst = true;
                for (std::size_t ii = 0; ii < strs.size(); ii++)
                {
                    if (rstr == strs[ii])
                    {
                        if (isFirst)
                        {
                            isFirst = false;
                        }
                        else
                        {
                            strs.erase(strs.begin() + ii);
                            ii --;
                        }
                    }
                }
                isReplicate = false;
            }
        }
    }
}

void removeDuplicate(FreqStringVector& fstrs, bool keepOrder)
{
    if ( 1 >= fstrs.size())
        return;
    if (!keepOrder)
    {
        std::sort(fstrs.begin(), fstrs.end(), FreqString::nameComparator);
        for (std::size_t i = 1; i < fstrs.size(); i++)
        {
            if (fstrs[i-1].getString() == fstrs[i].getString())
            {
                //fstrs[i-1].setFreq(fstrs[i-1].getFreq() + fstrs[i].getFreq());
                fstrs[i].remove();
            }
        }
        std::size_t j = 0;
        for (std::size_t i = 0; i < fstrs.size(); i++)
        {
            if (!fstrs[i].isRemoved())
            {
                if (i == j)
                    j++;
                else
                {
                    fstrs[j] = fstrs[i];
                    j++;
                    fstrs[i].remove();
                }
            }
        }
        fstrs.resize(j);
        return;
    }
    
    for (std::size_t i = 0; i < fstrs.size(); i++)
    {
        if (fstrs[i].isRemoved())
            continue;
        for (std::size_t j = 0; j < fstrs.size(); j++)
        {
            if (fstrs[j].isRemoved())
                continue;
            if (i == j)
                continue;
            if (fstrs[i].getString() == fstrs[j].getString())
            {
                //fstrs[i].setFreq(fstrs[i].getFreq() + fstrs[i].getFreq());
                //fstrs[j].remove();
                if (fstrs[i].getFreq() >= fstrs[j].getFreq())
                    fstrs[j].remove();
                else
                    fstrs[i].remove();
            }
        }
    }
    resize(fstrs);
}

void removeItem(FreqStringVector& strs, const std::string& str)
{
    for (std::size_t i = 0; i < strs.size(); i++)
    {
        std::string pstr= strs[i].getString();
        bool remove = true;
        for (std::size_t ii = 0; ii < pstr.size(); ii++)
        {
            if (isspace(pstr[ii]))
                continue;
            if (std::string::npos == str.find(pstr[ii]))
            {
                remove = false;
                break;
            }
        }
        if (remove)
            strs[i].remove();
        //if (str == strs[i].getString())
        //    strs[i].remove();
    }
    resize(strs);
}

bool isNeedRemove(const std::string& lv, const std::string& rv)
{
    bool remove = true;
    for (std::size_t i = 0; i < lv.size(); i++)
    {
        if (isspace(lv[i]))
            continue;
        if (std::string::npos == rv.find(lv[i]))
        {
            remove = false;
            break;
        }
    }
    if (std::string::npos != lv.find("男"))
    {
        if (std::string::npos != rv.find("女"))
            return true;
    }
    if (std::string::npos != lv.find("女"))
    {
        if (std::string::npos != rv.find("男"))
            return true;
    }
    return remove;
}

void resize(FreqStringVector& strs)
{
    std::size_t j = 0;
    for (std::size_t i = 0; i < strs.size(); i++)
    {
        if (!strs[i].isRemoved())
        {
            if (i == j)
                j++;
            else
            {
                strs[j] = strs[i];
                j++;
                strs[i].remove();
            }
        }
    }
    strs.resize(j);
}

int nBlank(const std::string& str)
{
    const char* pos = str.c_str();
    std::size_t size = str.size();
    int n = 0;
    for (std::size_t i = 0; i < size; i++, pos++)
    {
        if (' ' == *pos)
            n++;
    }
    return n;
}
using namespace izenelib::util;

static int editDistance(const izenelib::util::UString& source,const izenelib::util::UString& target)
{
    int n=source.length();
    int m=target.length();
    if (m==0) return n;
    if (n==0) return m;

    typedef vector< vector<int> >  Tmatrix;
    Tmatrix matrix(n+1);
    for(int i=0; i<=n; i++)  matrix[i].resize(m+1);
    for(int i=1; i<=n; i++) matrix[i][0]=i;
    for(int i=1; i<=m; i++) matrix[0][i]=i;

    for(int i=1; i<=n; i++)
    {
        const UCS2Char si=source[i-1];
        for(int j=1; j<=m; j++)
        {
            const UCS2Char dj=target[j-1];
            int cost;
            if(si==dj)
            {
                cost=0;
            }
            else
            {
                cost=1;
            }
            const int above=matrix[i-1][j]+1;
            const int left=matrix[i][j-1]+1;
            const int diag=matrix[i-1][j-1]+cost;
            matrix[i][j]=min(above,min(left,diag));
        }
    }
    return matrix[n][m];

}

int editDistance(const std::string& sv, const std::string& tv)
{
    izenelib::util::UString source(sv, izenelib::util::UString::UTF_8);
    izenelib::util::UString target(tv, izenelib::util::UString::UTF_8);
    return editDistance(source, target);
}

int strToInt(const std::string& str, boost::bimap<int, std::string>& bm)
{
    static int id = 0;
    boost::bimap<int, std::string>::right_const_iterator it;
    it = bm.right.find(str);
    if (bm.right.end() == it)
    {
        id++;
        bm.insert(boost::bimap<int, std::string>::value_type(id, str));
        return id;
    }
    else
        return it->second;
}

std::string intToStr(int n, boost::bimap<int, std::string>& bm)
{
    boost::bimap<int, std::string>::left_const_iterator it;
    it = bm.left.find(n);
    if (bm.left.end() == it)
    {
        return "";
    }
    return it->second;
}

void tuneByEditDistance(FreqStringVector& vector, const std::string& str)
{
    if (vector.size() <= 1)
        return;
    double freq = vector[0].getFreq();
    FreqStringVector sortVector;
    for (std::size_t i = 0; i < vector.size(); i++)
    {
        vector[i].setPosition(i);
        if (vector[i].getFreq() == freq)
        {
            FreqString v = vector[i];
            v.setFreq(editDistance(v.getString(), str));
            sortVector.push_back(v);
            //std::cout<<v.getString()<<" : "<<v.getFreq()<<"\n";
        }
        else
        {
            if (sortVector.empty())
                return;
            std::size_t pos = sortVector[0].getPosition();
            std::sort(sortVector.begin(), sortVector.end());
            for (std::size_t si = 0; si < sortVector.size(); si++)
            {
                //std::cout<<vector[pos + si].getString()<<" => "<<sortVector[si].getString()<<"\n";
                vector[pos + si].setString(sortVector[si].getString());
            }
            sortVector.clear();
            freq = vector[i].getFreq();
        }
    }
            
    if (sortVector.empty())
        return;
    std::size_t pos = sortVector[0].getPosition();
    std::sort(sortVector.begin(), sortVector.end());
    for (std::size_t si = 0; si < sortVector.size(); si++)
    {
        //std::cout<<vector[pos + si].getString()<<" => "<<sortVector[si].getString()<<"\n";
        vector[pos + si].setString(sortVector[si].getString());
    }
}

FreqString max(FreqStringVector& v)
{
    FreqString max("", 0);
    for (std::size_t i = 0; i < v.size(); i++)
    {
        if (max < v[i])
            max = v[i];
    }
    return max;
}

bool isEnglish(const std::string& userQuery)
{
    const char* pos = userQuery.c_str();
    const char* end = pos + userQuery.size();
    bool ret = true;
    for(; pos < end; pos++)
    {
        if (isalpha(*pos) || isblank(*pos))
        {
        }
        else
        {
            ret = false;
            break;
        }
    }
    return ret;
}

void removeSpace(const std::string& src, std::string& tar)
{
    for (std::size_t i = 0; i < src.size(); i++)
    {
        if (!isspace(src[i]))
            tar += src[i];
    }
}

}
}
}
