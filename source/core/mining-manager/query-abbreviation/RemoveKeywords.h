#ifndef REMOVE_KEYWORDS_H
#define REMOVE_KEYWORDS_H

#include <mining-manager/MiningManager.h>
#include <vector>
#include <string>

namespace sf1r
{

namespace RK
{

extern std::string INVALID;
class Token
{
public:
    Token(std::string token, double w, std::size_t l)
    {
        token_ = token;
        weight_ = w;
        location_ = l;
        isCenterToken_ = false;
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

    friend Token& operator+=(Token& lv, const Token& rv)
    {
        /*Token token;
        token.token_ = lv.token_ + rv.token_;
        token.weight_ = lv.weight_ + rv.weight_;
        token.location_ = lv.location_ + rv.location_;
        return token;*/
        if (&lv == & rv)
            return lv;
        lv.token_ += rv.token_;
        lv.weight_ += rv.weight_;
        lv.location_ = rv.location_ > lv.location_ ? lv.location_ : rv.location_;
        return lv;
    }

    friend Token operator+(const Token& lv, const Token& rv)
    {
        Token token("", 0.0, 0);
        token.token_ = lv.token_ + rv.token_;
        token.weight_ = lv.weight_ + rv.weight_;
        token.location_ = lv.location_ > rv.location_ ? rv.location_ : lv.location_;
        return token;
    }

private:
    std::string token_;
    double weight_;
    std::size_t location_;
    bool isCenterToken_;
};


typedef std::vector<Token> TokenArray;

class TokenRecommended
{

static bool Comp(const TokenArray& lv, const TokenArray& rv)
{
    return lv[0].weight() > rv[0].weight();
}

public:
    TokenRecommended()
    {
        tokenArrays_.clear();
        tIndex_ = std::numeric_limits<std::size_t>::max();
        aIndex_ = std::numeric_limits<std::size_t>::max();
    }
public:
    void push_back(TokenArray& tokens)
    {
        tokenArrays_.push_back(tokens);
    }
    
    void sort()
    {
        std::sort(tokenArrays_.begin(), tokenArrays_.end(), TokenRecommended::Comp);
    }
    
    const std::string& nextToken(bool lastSuccess)
    {
        if (lastSuccess)
        {
            if (std::numeric_limits<std::size_t>::max() == tIndex_)
                tIndex_ = 0;
            else
                tIndex_++;
            if (std::numeric_limits<std::size_t>::max() == aIndex_)
                aIndex_ = 0;

            if (aIndex_ >= tokenArrays_.size())
                return INVALID;
            if (tIndex_ < tokenArrays_[aIndex_].size())
                return tokenArrays_[aIndex_][tIndex_].token();
        }
       
        if (std::numeric_limits<std::size_t>::max() == aIndex_)
            aIndex_ = 0;
        else
            aIndex_ ++;
        tIndex_ = 0;
        
        if (aIndex_ >= tokenArrays_.size())
            return INVALID;
        if (tIndex_ >= tokenArrays_[aIndex_].size())
            return INVALID;
        return tokenArrays_[aIndex_][tIndex_].token();
    }

    friend std::ostream& operator<<(std::ostream& out, TokenRecommended& tokens)
    {
        for (std::size_t aIndex = 0; aIndex < tokens.tokenArrays_.size(); aIndex++)
        {
            out<<aIndex<<": ";
            for (std::size_t tIndex = 0; tIndex < tokens.tokenArrays_[aIndex].size(); tIndex++)
            {
                out<<tokens.tokenArrays_[aIndex][tIndex].token()<<" ";
            }
            out<<"\n";
        }
        return out;
    }
private:
    std::vector<TokenArray> tokenArrays_;
    std::size_t tIndex_;
    std::size_t aIndex_;
};

void generateTokens(TokenArray& tokens, std::string& keywords, MiningManager& miningManager);
void adjustWeight(TokenArray& tokens, std::string& keywords, MiningManager& miningManager);
void removeTokens(TokenArray& tokens, TokenRecommended& queries);
void queryAbbreviation(TokenRecommended& queries, std::string& keywords, MiningManager& miningManager);

}
} // end of namespace

#endif
