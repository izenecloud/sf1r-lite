#include "Parser.h"
#include <iostream>
namespace sf1r
{
namespace Recommend
{

const Parser::iterator& Parser::begin()
{
    return ++it_;
}

const Parser::iterator& Parser::end() const
{
    static Parser::iterator it(NULL, -1);
    return it;
}

const Parser::iterator& Parser::iterator::operator++()
{
    if (p_->next())
    {
        uuid_++;
        return *this;
    }
    else
    {
        uuid_ = -1;
        return *this;
        //return p_->end();
    }
}

const UserQuery& Parser::iterator::operator*() const
{
    return p_->get();
}

const UserQuery* Parser::iterator::operator->() const
{
    return &(p_->get());
}

bool Parser::iterator::operator== (const Parser::iterator& other) const
{
    return uuid_ == other.uuid_;
}

bool Parser::iterator::operator!= (const Parser::iterator& other) const
{
    return uuid_ != other.uuid_;
}
}
}
