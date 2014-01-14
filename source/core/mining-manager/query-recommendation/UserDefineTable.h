#ifndef SF1R_RECOMMEND_USER_DEFINE_TABLE_H
#define SF1R_RECOMMEND_USER_DEFINE_TABLE_H

#include <boost/unordered_map.hpp>
#include <vector>
#include <util/ustring/UString.h>

namespace sf1r
{
namespace Recommend
{
class UserDefineTable
{
public:
    UserDefineTable(const std::string& workdir);
    ~UserDefineTable();

public:
    void build(const std::string& path = "");
    bool isNeedBuild(const std::string& path = "") const;
    bool isValid(const std::string& f) const;
    
    bool search(const std::string& userQuery, std::string& correct) const;
    
    void flush() const;
    void clear();

private:
    void buildFromFile(const std::string& f);

private:
    boost::unordered_map<std::string, std::string> table_;
    std::time_t timestamp_;
    std::string workdir_;
};
}
}
#endif
