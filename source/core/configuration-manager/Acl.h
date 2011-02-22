#ifndef INCLUDE_SF1V5_CONFIGURATION_MANAGER_ACL_H
#define INCLUDE_SF1V5_CONFIGURATION_MANAGER_ACL_H

#include <boost/unordered_set.hpp>

namespace sf1r {

/**
 * @brief ACL rule
 *
 * A rule contains two token list:
 *
 * - allow
 * - deny
 */
class Acl
{
public:
    typedef boost::unordered_set<std::string> token_set_type;
    typedef token_set_type::const_iterator const_iterator;
    typedef const_iterator iterator;

    static void insertTokensTo(const std::string& tokens, token_set_type& set);

    Acl();

    ~Acl();

    Acl& allow(const std::string& tokens);

    Acl& deny(const std::string& tokens);

    Acl& merge(const Acl& anotherAcl);

    // @brief check whether the user tokens is allowed
    bool check(const std::string& userTokens) const;

    bool check(const token_set_type& userTokens) const;

    const_iterator allowedTokensBegin() const
    {
        return allowedTokens_.begin();
    }
    const_iterator allowedTokensEnd() const
    {
        return allowedTokens_.end();
    }

    const_iterator deniedTokensBegin() const
    {
        return deniedTokens_.begin();
    }
    const_iterator deniedTokensEnd() const
    {
        return deniedTokens_.end();
    }

    void clear()
    {
        allowedTokens_.clear();
        deniedTokens_.clear();
    }

    bool empty() const
    {
        return allowedTokens_.empty() && deniedTokens_.empty();
    }

    void swap(Acl& anotherAcl)
    {
        allowedTokens_.swap(anotherAcl.allowedTokens_);
        deniedTokens_.swap(anotherAcl.deniedTokens_);
    }

private:
    token_set_type allowedTokens_;
    token_set_type deniedTokens_;
};

inline void swap(Acl& a, Acl& b)
{
    a.swap(b);
}

}

#endif // INCLUDE_SF1V5_CONFIGURATION_MANAGER_ACL_H
