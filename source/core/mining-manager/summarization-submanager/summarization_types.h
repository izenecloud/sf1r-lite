#ifndef SF1R_MINING_MANAGER_SUMMARIZATION_TYPES_H
#define SF1R_MINING_MANAGER_SUMMARIZATION_TYPES_H

#include <common/type_defs.h>

#include <util/ustring/UString.h>

#include <string>
#include <vector>
#include <map>

namespace sf1r
{

typedef uint128_t KeyType;
typedef izenelib::util::UString ContentType;
typedef float ScoreType;

typedef std::map<uint32_t, std::pair<ContentType, ScoreType> > CommentCacheItemType;

}

MAKE_FEBIRD_SERIALIZATION(sf1r::CommentCacheItemType)

#endif
