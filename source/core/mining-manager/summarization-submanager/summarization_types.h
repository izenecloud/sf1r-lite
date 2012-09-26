#ifndef SF1R_MINING_MANAGER_SUMMARIZATION_TYPES_H
#define SF1R_MINING_MANAGER_SUMMARIZATION_TYPES_H

#include <common/type_defs.h>

#include <util/ustring/UString.h>

#include <string>
#include <vector>
#include <map>
#include <boost/tuple/tuple.hpp>

namespace sf1r
{

typedef uint128_t KeyType;
typedef izenelib::util::UString ContentType;
typedef izenelib::util::UString AdvantageType;
typedef izenelib::util::UString DisadvantageType;
typedef float ScoreType;

struct CommentSummaryT
{
    ContentType content;
    AdvantageType advantage;
    DisadvantageType disadvantage;
    ScoreType  score;
    
    template<class DataIO>
    friend void DataIO_loadObject(DataIO& dio, CommentSummaryT& x);
    template<class DataIO>
    friend void DataIO_saveObject(DataIO& dio, const CommentSummaryT& x);

};
//typedef std::pair<ContentType, ScoreType> CommentSummaryT;
//typedef boost::tuple<ContentType, AdvantageType, DisadvantageType, ScoreType> CommentSummaryT;

template<class DataIO>
inline void DataIO_loadObject(DataIO& dio, CommentSummaryT& x)
{
    dio & x.content;
    dio & x.advantage;
    dio & x.disadvantage;
    dio & x.score;
}

template<class DataIO>
inline void DataIO_saveObject(DataIO& dio, const CommentSummaryT& x)
{
    dio & x.content;
    dio & x.advantage;
    dio & x.disadvantage;
    dio & x.score;
}

typedef std::map<uint32_t, CommentSummaryT > CommentCacheItemType;

}

MAKE_FEBIRD_SERIALIZATION(sf1r::CommentSummaryT)
MAKE_FEBIRD_SERIALIZATION(sf1r::CommentCacheItemType)

#endif
