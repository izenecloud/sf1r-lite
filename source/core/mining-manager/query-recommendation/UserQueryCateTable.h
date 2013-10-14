#ifndef USER_QUERY_CATE_TABLE_H
#define USER_QUERY_CATE_TABLE_H

#include <boost/unordered_map.hpp>
#include <queue>
#include <boost/function.hpp>

namespace sf1r
{
namespace Recommend
{


typedef boost::function<bool(const std::string&, const std::string&)> CateEqualer;

class UserQueryCateTable
{
typedef std::pair<std::string, uint32_t> CateT;
typedef std::vector<CateT> CateV;
public:
    UserQueryCateTable(const std::string& workdir);
    ~UserQueryCateTable();

public:
    void insert(const std::string& userQuery, const std::string cate, uint32_t freq);
    void search(const std::string& userQuery, std::vector<std::pair<std::string, uint32_t> >& results) const;
    bool cateEqual(const std::string& lv, const std::string& rv) const;
    
    void flush() const;
    void clear();

    friend std::ostream& operator<<(std::ostream& out, const UserQueryCateTable& tc);
    friend std::istream& operator>>(std::istream& in,  UserQueryCateTable& tc);

private:
    boost::unordered_map<std::string, CateV> table_;
    std::string workdir_;
/*
public:
    class UserQuery
    {
    public:
        UserQuery(uint32_t freq, const std::string& userQuery)
            : freq_(freq)
            , userQuery_(userQuery)
        {
        }
    public:
        const std::string& userQuery() const
        {
            return userQuery_;
        }

        const uint32_t freq() const
        {
            return freq_;
        }

        friend bool operator<(const UserQuery& lv, const UserQuery& rv);
    private:
        uint32_t freq_;
        std::string userQuery_;
    };

    typedef std::priority_queue<UserQuery> TopNQueue;
    typedef std::list<UserQueryCateTable::UserQuery> UserQueryList;

public:
    UserQueryCateTable(const std::string& workdir);
    ~UserQueryCateTable();

public:
    void insert(const std::string& userQuery, const std::string& cate, uint32_t freq);
    void topNUserQuery(const std::string& cate, const uint32_t N, UserQueryList& userQuery) const;
    
    void flush();
    friend std::ostream& operator<<(std::ostream& out, const UserQueryCateTable& uqc);
    friend std::istream& operator>>(std::istream& in,  UserQueryCateTable& uqc);
private:
    boost::unordered_map<std::string, TopNQueue> topNUserQuery_;
    std::string workdir_;
    static std::size_t N;
*/
};

}
}
#endif
