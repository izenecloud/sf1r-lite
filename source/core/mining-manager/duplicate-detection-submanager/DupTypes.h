/*
 * DupTypes.h
 *
 *  Created on: 2009-1-15
 *      Author: jinglei
 */

#ifndef DUPTYPES_H_
#define DUPTYPES_H_

#include <ext/hash_map>
#include <util/CBitArray.h>

namespace sf1r
{
/**
 * The types for dup detection.
 */
enum DUP_ALG {BRODER=0,CHARIKAR=1};
//typedef __gnu_cxx::hash_multimap<uint32_t,NearDuplicateSignature > MULTI_MAP_T;
typedef __gnu_cxx::hash_map<unsigned int, izenelib::util::CBitArray> DOC_BIT_MAP;
typedef pair<unsigned int, unsigned int> DUP_PAIR;
typedef set<DUP_PAIR> DUP_PAIR_SET;
}
#endif /* DUPTYPES_H_ */
