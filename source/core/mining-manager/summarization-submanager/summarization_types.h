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

//struct CommentSummaryT
//{
//    ContentType content;
//    //AdvantageType advantage;
//    //DisadvantageType disadvantage;
//    ScoreType  score;
//};
//typedef std::pair<ContentType, ScoreType> CommentSummaryT;
typedef boost::tuple<ContentType, AdvantageType, DisadvantageType, ScoreType> CommentSummaryT;

typedef std::map<uint32_t, CommentSummaryT > CommentCacheItemType;

}

MAKE_FEBIRD_SERIALIZATION(sf1r::CommentCacheItemType)

#endif
