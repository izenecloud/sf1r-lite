#ifndef REMOVE_KEYWORDS_H
#define REMOVE_KEYWORDS_H

#include <mining-manager/MiningManager.h>
#include <vector>
#include <string>

namespace sf1r
{

namespace RK
{

class Token
{
public:
    Token(std::string token, double w, std::size_t l)
    {
        token_ = token;
        weight_ = w;
        location_ = l;
        isCenterToken_ = false;
        tag_ = false;
    }

    const std::string& token() const
    {
        return token_;
    }

    double weight() const
    {
        return weight_;
    }

    void setWeight(double w)
    {
        weight_ = w;
    }

    std::size_t location() const
    {
        return location_;
    }

    void scale(double factor)
    {
        weight_ *= factor;
    }

    void combine(double weight)
    {
        weight_ += weight;
    }

    bool isCenterToken() const
    {
        return isCenterToken_;
    }

    void setCenterToken(bool is)
    {
        isCenterToken_ = is;
    }

    bool isLeft(Token& left)
    {
        if (left.location_ + left.token_.size() == location_ )
            return true;
        return false;
    }

    bool isRight(Token& right)
    {
        if (right.location_ == location_ + token_.size())
            return true;
        return false;
    }

    Token& operator+(const Token& token)
    {
        /*Token token;
        token.token_ = lv.token_ + rv.token_;
        token.weight_ = lv.weight_ + rv.weight_;
        token.location_ = lv.location_ + rv.location_;
        return token;*/
        token_ += token.token_;
        weight_ += token.weight_;
        location_ = token.location_ > location_ ? location_ : token.location_;
        return *this;
    }

    void setTag(bool tag)
    {
        tag_ = tag;
    }

    bool isTag() const
    {
        return tag_;
    }
private:
    std::string token_;
    double weight_;
    std::size_t location_;
    bool isCenterToken_;
    bool tag_;
};
typedef std::vector<Token> TokenArray;

void generateTokens(TokenArray& tokens, std::string& keywords, MiningManager& miningManager);
void adjustWeight(TokenArray& tokens, std::string& keywords, MiningManager& miningManager);
void removeTokens(TokenArray& tokens, TokenArray& queries);

void queryAbbreviation(TokenArray& queries, std::string& keywords, MiningManager& miningManager)
{
    TokenArray tokens;
    generateTokens(tokens, keywords, miningManager);
    adjustWeight(tokens, keywords, miningManager);
    removeTokens(tokens, queries);
}

}
} // end of namespace

#endif
