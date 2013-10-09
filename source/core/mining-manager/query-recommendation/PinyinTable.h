#ifndef PINYIN_TABLE_H
#define PINYIN_TABLE_H

#include "parser/Parser.h"
#include <boost/unordered_map.hpp>

namespace sf1r
{
namespace Recommend
{
class PinyinTable
{
public:
    PinyinTable(const std::string& workdir);
    ~PinyinTable();
public:
    void insert(const std::string& userQuery, const std::string& pinyin, uint32_t freq);
    void search(const std::string& userQuery, UserQueryList& uqlist) const;
    
    void flush() const;
    void clear();
    friend std::ostream& operator<<(std::ostream& out, const PinyinTable& tc);
    friend std::istream& operator>>(std::istream& in,  PinyinTable& tc);
private:
    boost::unordered_map<std::string, UserQueryList> pinyinTable_;
    std::string workdir_;
};
}
}
#endif
