/**
 * @file core/configuration-manager/Acl.cpp
 * @author Ian Yang
 * @date Created <2011-01-25 13:28:36>
 */
#include "Acl.h"

#include <boost/algorithm/string/find_iterator.hpp>
#include <boost/algorithm/string/finder.hpp>
#include <boost/algorithm/string/compare.hpp>

namespace sf1r
{
const std::string Acl::STOP_SERVICE_TOKEN = "@@ALL@@";
Acl::Acl()
        : allowedTokens_()
        , deniedTokens_()
{
    // empty
}

Acl::~Acl()
{
    // empty
}

Acl& Acl::allow(const std::string& tokens)
{
    insertTokensTo(tokens, allowedTokens_);

    return *this;
}

Acl& Acl::deny(const std::string& tokens)
{
    insertTokensTo(tokens, deniedTokens_);

    return *this;
}

Acl& Acl::deleteTokenFromAllow(const std::string& token)
{
    deleteTokenFrom(token, allowedTokens_);

    return *this;
}

Acl& Acl::deleteTokenFromDeny(const std::string& token)
{
    deleteTokenFrom(token, deniedTokens_);

    return *this;
}

Acl& Acl::merge(const Acl& anotherAcl)
{
    allowedTokens_.insert(anotherAcl.allowedTokensBegin(),
                          anotherAcl.allowedTokensEnd());
    deniedTokens_.insert(anotherAcl.deniedTokensBegin(),
                         anotherAcl.deniedTokensEnd());

    return *this;
}

bool Acl::check(const std::string& userTokens) const
{
    if (empty())
    {
        return true;
    }

    token_set_type userTokenSet;
    insertTokensTo(userTokens, userTokenSet);

    return check(userTokenSet);
}

bool Acl::checkDenyList() const
{
    bool returnvalue = true;
    for (const_iterator it = deniedTokensBegin();
                 it != deniedTokensEnd(); ++it)
    {
         if (*it == STOP_SERVICE_TOKEN)
         {
              returnvalue = false;
              break;
         }
    }
    return returnvalue;
}

bool Acl::check(const token_set_type& userTokens) const
{
    for (const_iterator it = deniedTokensBegin();
            it != deniedTokensEnd(); ++it)
    {
        // false if user holds any denied token
        if (userTokens.count(*it) != 0)
        {
            return false;
        }
    }

    // if allow is not empty, user must hold at least one token
    if (!allowedTokens_.empty())
    {
        for (const_iterator it = allowedTokensBegin();
                it != allowedTokensEnd(); ++it)
        {
            if (userTokens.count(*it) != 0)
            {
                return true;
            }
        }

        // user has none allowed tokens
        return false;
    }
    else
    {
        return true;
    }
}

void Acl::insertTokensTo(const std::string& tokens, token_set_type& set)
{
    typedef boost::algorithm::split_iterator<std::string::const_iterator>
    string_split_iterator;
    using boost::algorithm::first_finder;
    using boost::algorithm::is_equal;

    string_split_iterator itBegin(tokens, first_finder(",", is_equal()));
    string_split_iterator itEnd;
    std::string token;

    for (string_split_iterator it = itBegin; it != itEnd; ++it)
    {
        if (it->begin() != it->end())
        {
            token.assign(it->begin(), it->end());
            set.insert(token);
        }
    }
}

void Acl::deleteTokenFrom(const std::string& token, token_set_type& set)
{
    set.erase(token);
}

} // namespace sf1r
