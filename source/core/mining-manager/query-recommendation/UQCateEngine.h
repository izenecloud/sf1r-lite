#ifndef UQ_CATE_ENGINE_H
#define UQ_CATE_ENGINE_H

#include <boost/unordered_map.hpp>
#include <queue>
#include <boost/function.hpp>

namespace sf1r
{
namespace Recommend
{


typedef boost::function<bool(const std::string&, const std::string&)> CateEqualer;

class UQCateEngine
{
typedef std::pair<std::string, uint32_t> CateT;
typedef std::vector<CateT> CateV;
public:
    UQCateEngine();
    ~UQCateEngine();
    
    static UQCateEngine& getInstance()
    {
        static UQCateEngine engine;
        return engine;
    }

public:
    void lock(void* identifier);
    void flush(const void* identifier = NULL) const;
    void clear(const void* identifier = NULL);

    void insert(const std::string& userQuery, const std::string cate, uint32_t freq, const void* identifier = NULL);
    void search(const std::string& userQuery, std::vector<std::pair<std::string, uint32_t> >& results) const;
    bool cateEqual(const std::string& userQuery, const std::string& v, double threshold) const;
    
    friend std::ostream& operator<<(std::ostream& out, const UQCateEngine& tc);
    friend std::istream& operator>>(std::istream& in,  UQCateEngine& tc);

private:
    boost::unordered_map<std::string, CateV> table_;
    void* owner_;
public:
    static std::string workdir;
};

}
}
#endif
