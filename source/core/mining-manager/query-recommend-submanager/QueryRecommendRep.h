///
/// @file QueryRecommendRep.h
/// @brief The representation class for query recommend
/// @author Jia Guo <guojia@gmail.com>
/// @date Created 2009-12-21
/// @date Updated 2009-12-21
///

#ifndef QUERYRECOMMENDREP_H_
#define QUERYRECOMMENDREP_H_

namespace sf1r
{

/// @brief The memory representation form of a Query Recommend.
class QueryRecommendRep
{
public:
    QueryRecommendRep():
            recommendQueries_(), scores_()
    {
    }
    ~QueryRecommendRep()
    {
    }
    QueryRecommendRep(const QueryRecommendRep& rhs):
            recommendQueries_(rhs.recommendQueries_), scores_(rhs.scores_)
    {
    }

    QueryRecommendRep& operator=(const QueryRecommendRep& rhs)
    {
        recommendQueries_ = rhs.recommendQueries_;
        scores_ = rhs.scores_;
        return *this;
    }

    std::deque<izenelib::util::UString > recommendQueries_;

    std::vector<float > scores_;

    std::string error_;

};
}


#endif
