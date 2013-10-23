#ifndef FILTER_H
#define FILTER_H

#include <string>
#include <util/DynamicBloomFilter.h>

namespace sf1r
{
namespace Recommend
{
class Filter
{
typedef izenelib::util::DynamicBloomFilter<std::string> BloomFilter;
public:
    Filter(const std::string& workdir);
    ~Filter();
public:
    void buildFilter(const std::string& path = "");
    bool isNeedBuild(const std::string& path = "") const;
    bool isFilter(const std::string& userQuery) const;
    bool isValid(const std::string& f) const;
    
    void flush() const;
    void clear();

    friend std::ostream& operator<<(std::ostream& out, const Filter& filter);
    friend std::istream& operator>>(std::istream& in,  Filter& filter);

private:
    void buildFromFile(const std::string& f);
    DISALLOW_COPY_AND_ASSIGN(Filter);

private:
    BloomFilter* bf_;
    std::time_t timestamp_;
    const std::string workdir_;
};
}
}

#endif
