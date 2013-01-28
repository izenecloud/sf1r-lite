#include "wavelet_matrix.hpp"

using namespace std;
using namespace wavelet_matrix;
namespace sf1r
{

void TDTWaveletMatrix::QuantileRangeAll(uint64_t begin_pos, uint64_t end_pos, vector<uint64_t>& ret,const BitTrie& filter) const
{
    uint64_t val;
    if ((end_pos > length_ || begin_pos >= end_pos))
    {
        //pos = NOTFOUND;
        //val = NOTFOUND;
        return;
    }

    val = 0;

    size_t i = 0;
    //uint64_t k=0;Schema
    //cout<<"alphabet_bit_num_"<<alphabet_bit_num_<<endl;
    QuantileRangeEach(begin_pos, end_pos, i,val, end_pos - begin_pos+1,ret,filter.Root_);

    //QuantileRangeEach_NonRecursive(begin_pos, end_pos, i,beg_node ,end_node, val,ret);


}
void TDTWaveletMatrix::QuantileRangeEach(uint64_t begin_pos, uint64_t end_pos, size_t i,uint64_t val,int k,vector<uint64_t>& ret,BitNode* node) const
{

    if(i==alphabet_bit_num_)
    {
        ret.push_back(val);
    }
    else
    {
        const BitArray& ba = bit_arrays_[i];
        uint64_t begin_zero, end_zero;
        begin_zero = ba.Rank(0, begin_pos);
        end_zero = ba.Rank(0, end_pos);
        uint64_t zero_bits = end_zero - begin_zero;
        if (zero_bits>0)
        {
            if(node->ZNext_)
                QuantileRangeEach(begin_zero, end_zero, i+1, (val << 1) ,zero_bits,ret,node->ZNext_);//
        }
        if (k-zero_bits>0)
        {
            begin_pos += zero_counts_[i] - begin_zero;
            end_pos += zero_counts_[i] - end_zero;
            if(node->ONext_)
                QuantileRangeEach(begin_pos, end_pos, i+1, (val << 1) + 1,k-zero_bits, ret,node->ONext_);//
        }

    }

}

}
