#ifndef STRING_UTIL_H
#define STRING_UTIL_H

#include <boost/algorithm/string.hpp>
#include <boost/functional/hash.hpp>
#include <vector>
#include <limits>

#include <boost/bimap.hpp>

namespace sf1r
{
namespace Recommend
{
typedef std::vector<std::string> StringVector;

class FreqString
{
public:
    FreqString()
        : str_("")
        , freq_(0.0)
        , pos_(0)
    {
    }
    
    FreqString(const std::string& str, double freq)
        : str_(str)
        , freq_(freq)
        , pos_(0)
    {
    }
    
    FreqString(const std::string& str, double freq, std::size_t pos)
        : str_(str)
        , freq_(freq)
        , pos_(pos)
    {
    }


    FreqString(const FreqString& fstr)
    {
        str_ = fstr.str_;
        freq_ = fstr.freq_;
        pos_ = fstr.pos_;
    }
public:
    void setString(const std::string& str)
    {
        str_ = str;
    }

    const std::string& getString() const
    {
        return str_;
    }

    void setFreq(double freq)
    {
        freq_ = freq;
    }

    double getFreq() const
    {
        return freq_;
    }

    std::size_t getPosition() const
    {
        return pos_;
    }

    void setPosition(std::size_t pos)
    {
        pos_ = pos;
    }

    FreqString& operator=(const FreqString& fstr)
    {
        str_ = fstr.str_;
        freq_ = fstr.freq_;
        pos_ = fstr.pos_;
        return *this;
    }
    
    void remove()
    {
        pos_ = std::numeric_limits<std::size_t>::max();
    }

    bool isRemoved() const
    {
        return pos_ == std::numeric_limits<std::size_t>::max();
    }

    friend bool operator<(const FreqString& lv, const FreqString& rv)
    {
        return lv.freq_ < rv.freq_;
    }
   
    friend std::ostream& operator<<(std::ostream& out, const FreqString& v)
    {
        out<<v.str_<<" :: "<<v.freq_<<" :: "<<v.pos_<<"\n";
        return out;
    }
    
    static bool nameComparator(const FreqString& lv, const FreqString& rv)
    {
        return lv.str_ > rv.str_;
    }
private:
    std::string str_;
    double freq_;
    std::size_t pos_;
};

typedef std::vector<FreqString> FreqStringVector;

namespace StringUtil
{
std::string& normalize(std::string& str);
void removeDuplicate(StringVector& strs);
void removeDuplicate(FreqStringVector& strs, bool keepOrder = true);
void removeItem(FreqStringVector& strs, const std::string& str);
void resize(FreqStringVector& strs);

int nBlank(const std::string& str);
int editDistance(const std::string& sv, const std::string& tv);

typedef boost::hash<const std::string> HashFunc;

int strToInt(const std::string& str, boost::bimap<int, std::string>& bm);
std::string intToStr(int n, boost::bimap<int, std::string>& bm);

void tuneByEditDistance(FreqStringVector& vector, const std::string& str);

FreqString max(FreqStringVector& v);

bool isEnglish(const std::string& userQuery);
}
}
}

#endif
