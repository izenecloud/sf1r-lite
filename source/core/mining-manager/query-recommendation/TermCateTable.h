#ifndef TERM_CATE_TABLE_H
#define TERM_CATE_TABLE_H
#include <boost/unordered_map.hpp>
#include <boost/bimap.hpp>

namespace sf1r
{
namespace Recommend
{
class TermCateTable
{
public:
    typedef int CateId;
    typedef uint32_t Freq;
    
    class Cate : public std::pair<CateId, Freq>
    {
    public:
        Cate()
        {
        }
        
        Cate(CateId id, Freq freq)
        {
            first = id;
            second = freq;
        }
        
        
        Cate& operator=(const Cate& p)
        {
            first = p.first;
            second = p.second;
            return *this;
        }

        friend bool operator<(const Cate& lv, const Cate& rv);
    };
    class CateIdList : public std::list<Cate>
    {
    public:
       friend CateIdList& operator+=(CateIdList& lv, const CateIdList& rv);
       friend std::ostream& operator<<(std::ostream& out, const CateIdList& v);
    };

public:
    TermCateTable(const std::string& workdir);
    ~TermCateTable();
public:
    void insert(const std::string& term, const std::string& cate, const uint32_t fre);
    void category(const std::string& term, CateIdList& cates);
    void flush();

    std::string cateIdToCate(CateId id); 
    
    friend std::ostream& operator<<(std::ostream& out, const TermCateTable& tc);
    friend std::istream& operator>>(std::istream& in,  TermCateTable& tc);
private:
    boost::unordered_map<std::string, CateIdList> tcTable_;
    boost::bimap<int, std::string> idmap_;
    //top N terms for each category
    const std::string workdir_;
};
}
}

#endif
