#ifndef PREFIX_TABLE_H
#define PREFIX_TABLE_H

#include <boost/unordered_map.hpp>
#include <vector>
#include <util/ustring/UString.h>
#include "parser/Parser.h"

namespace sf1r
{
namespace Recommend
{
class PrefixTable
{
typedef std::vector<UserQueryList> LeveledUQL;

public:
    PrefixTable(const std::string& workdir);
    ~PrefixTable();

public:
    void insert(const std::string& userQuery, uint32_t freq);
    void search(const std::string& userQuery, UserQueryList& uqlist) const;
    
    void flush() const;
    void clear();

    static bool isPrefixEnglish(const std::string& userQuery);

    friend std::ostream& operator<<(std::ostream& out, const PrefixTable& tc);
    friend std::istream& operator>>(std::istream& in,  PrefixTable& tc);

private:
    static void prefix(const izenelib::util::UString& userQuery, izenelib::util::UString& pref);
private:
    boost::unordered_map<std::string, LeveledUQL> table_;
    static std::size_t PREFIX_SIZE;
    std::string workdir_;
};
}
}
#endif
