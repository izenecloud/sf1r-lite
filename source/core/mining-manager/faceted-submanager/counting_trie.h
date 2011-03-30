///
/// @file counting_trie.h
/// @brief counting trie
/// @author Jia Guo <guojia@gmail.com>
/// @date Created 2010-12-23
///

#ifndef SF1R_COUNTING_TRIE_H_
#define SF1R_COUNTING_TRIE_H_

#include <boost/serialization/access.hpp>
#include <boost/serialization/serialization.hpp>
#include <common/type_defs.h>
#include <common/ResultType.h>
#include "faceted_types.h"
NS_FACETED_BEGIN


class CountingTrie
{

    typedef uint32_t KeyType;
    typedef uint32_t TermIdType;
    typedef uint32_t ValueType;
public:
    CountingTrie();
    void Append(const std::vector<TermIdType>& terms);
    ValueType Count(const std::vector<TermIdType>& terms);


private:
    izenelib::am::rde_hash<std::pair<KeyType,TermIdType>, std::pair<KeyType,ValueType> > trie_;
    KeyType key_;


};
NS_FACETED_END
#endif
