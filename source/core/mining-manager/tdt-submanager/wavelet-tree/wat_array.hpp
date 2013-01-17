#ifndef SF1_WATARRAY_WATARRAY_HPP_
#define SF1_WATARRAY_WATARRAY_HPP_

#include "bit_trie.hpp"
#include <am/succinct/wat_array/wat_array.hpp>

namespace sf1r
{
class TDTWatArray : public wat_array::WatArray
{
public:
    //add qian wang
    void QuantileRangeEach(uint64_t begin_pos, uint64_t end_pos, size_t i,uint64_t beg_node ,uint64_t end_node, uint64_t val,vector<uint64_t>& ret,BitNode* node) const;

    void QuantileRangeEach_NonRecursive(uint64_t begin_pos, uint64_t end_pos, size_t i,uint64_t beg_node ,uint64_t end_node, uint64_t val,vector<uint64_t>& ret) const;

    void QuantileRangeAll(uint64_t begin_pos, uint64_t end_pos, std::vector<uint64_t>& ret,const BitTrie& filter) const;
};

}

#endif // WASEQ_WASEQ_HPP_
