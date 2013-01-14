#ifndef SF1_WAVELET_MATRIX_WAVELET_MATRIX_HPP_
#define SF1_WAVELET_MATRIX_WAVELET_MATRIX_HPP_

#include "bit_trie.hpp"
#include <am/succinct/wat_array/wavelet_matrix.hpp>

namespace sf1r
{

class MyWaveletMatrix : public wavelet_matrix::WaveletMatrix
{
public:
    void QuantileRangeEach(uint64_t begin_pos, uint64_t end_pos, size_t i, uint64_t val,int k,vector<uint64_t>& ret,BitNode* node) const;

    void QuantileRangeAll(uint64_t begin_pos, uint64_t end_pos, vector<uint64_t>& ret,const BitTrie& filter) const;
};

}


#endif // WAVELET_MATRIX_WAVELET_MATRIX_HPP_
