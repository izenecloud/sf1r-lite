#ifndef PARSER_H
#define PARSER_H

#include <string>
#include <unistd.h>
#include <stdint.h>
#include <list>

namespace sf1r
{
namespace Recommend
{
class UserQuery
{
public:
    UserQuery(const std::string& userQuery, const std::string& category, const uint32_t freq)
        : userQuery_(userQuery)
#ifndef DISABEL_CATEGORY
        , category_(category)
#endif
        , freq_(freq)
    {
    }
    
    UserQuery(const std::string& userQuery, const uint32_t freq)
        : userQuery_(userQuery)
#ifndef DISABEL_CATEGORY
        , category_("")
#endif
        , freq_(freq)
    {
    }

    UserQuery()
        : userQuery_("")
#ifndef DISABEL_CATEGORY
        , category_("")
#endif
        , freq_(0)
    {
    }

public:
    const std::string& userQuery() const
    {
        return userQuery_;
    }

    void setUserQuery(const std::string& userQuery)
    {
        userQuery_ = userQuery;
    }

    const std::string& category() const
    {
#ifndef DISABEL_CATEGORY
        return category_;
#endif
    }

    void setCategory(const std::string& category)
    {
#ifndef DISABEL_CATEGORY
        category_ = category;
#endif
    }

    const double freq() const
    {
        return freq_;
    }

    void setFreq(const double freq)
    {
        freq_ = freq;
    }

    void reset()
    {
        userQuery_ = "";
#ifndef DISABEL_CATEGORY
        category_  = "";
#endif
        freq_      = 0;
    }
private:
    std::string userQuery_;
#ifndef DISABEL_CATEGORY
    std::string category_;
#endif
    double freq_;
};

typedef std::list<UserQuery> UserQueryList;

class Parser
{
public:
    class iterator
    {
    public:
        iterator(Parser* p = NULL, uint32_t uuid = 0)
            : p_(p)
            , uuid_(uuid)
        {
        }
    
    public:
        const iterator& operator++();
        bool operator== (const iterator& other) const;
        bool operator!= (const iterator& other) const;
        iterator& operator=(const iterator& other);
        const UserQuery& operator*() const;
        const UserQuery* operator->() const; 
        
        void setParser(Parser* p)
        {
            p_ = p;
        }

    private:
        Parser* p_;
        uint32_t uuid_;
    };
public:
    Parser()
    {
        it_.setParser(this);
        //it_ = new Parser::iterator(this);
    }

    virtual ~Parser()
    {
        //if (NULL != it_)
        //{
        //    delete it_;
        //    it_ = NULL;
        //}
    }
public:
    virtual void load(const std::string& path) = 0;
    virtual bool next() = 0;
    virtual const UserQuery& get() const = 0;
    virtual const iterator& begin();
    virtual const iterator& end() const;

private:
    Parser::iterator it_;
};
}
}

#endif
