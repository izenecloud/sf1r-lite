#include "wat_array.hpp"

#include <queue>
#include <algorithm>
#include <stack>

using namespace std;
using namespace wat_array;
namespace sf1r
{

class Parameter
{
public:
    Parameter()
    {
    }
    Parameter(uint64_t bp, uint64_t ep, size_t ii,uint64_t bn ,uint64_t en, uint64_t v)
        :begin_pos(bp), end_pos(ep), i(ii), beg_node(bn), end_node(en), val(v)
    {
    }
    ~Parameter()
    {
    }
public:
    uint64_t begin_pos;
    uint64_t end_pos;
    size_t i;
    uint64_t beg_node;
    uint64_t end_node;
    uint64_t val;
};


void TDTWatArray::QuantileRangeAll(uint64_t begin_pos, uint64_t end_pos, vector<uint64_t>& ret,const BitTrie& filter) const
{
    uint64_t val;
    if (end_pos > length_ || begin_pos >= end_pos)
    {
        //pos = NOTFOUND;
        //val = NOTFOUND;
        return;
    }

    val = 0;
    uint64_t beg_node = 0;
    uint64_t end_node = length_;
    size_t i = 0;
    //uint64_t k=0;

    QuantileRangeEach(begin_pos, end_pos, i,beg_node ,end_node, val,ret,filter.Root_);

    //QuantileRangeEach_NonRecursive(begin_pos, end_pos, i,beg_node ,end_node, val,ret);


}
void TDTWatArray::QuantileRangeEach(uint64_t begin_pos, uint64_t end_pos, size_t i,uint64_t beg_node ,uint64_t end_node, uint64_t val,vector<uint64_t>& ret,BitNode* node) const
{
    //cout<<"QuantileRangeEach"<<"begin_pos"<<begin_pos<<"end_pos"<<end_pos<<"i"<<i<<"beg_node"<<beg_node<<"end_node"<<end_node<<"val"<<val<<"ret"<<ret.size()<<"grade"<<node->grade_<<endl;
    if(i==bit_arrays_.size())
    {
        ret.push_back(val);
    }
    else
    {
        const BitArray& ba = bit_arrays_[i];
        uint64_t beg_node_zero = ba.Rank(0, beg_node);
        uint64_t end_node_zero = ba.Rank(0, end_node);
        uint64_t beg_node_one  = beg_node - beg_node_zero;
        uint64_t beg_zero  = ba.Rank(0, begin_pos);
        uint64_t end_zero  = ba.Rank(0, end_pos);
        uint64_t beg_one   = begin_pos - beg_zero;
        uint64_t end_one   = end_pos - end_zero;
        uint64_t boundary  = beg_node + end_node_zero - beg_node_zero;


        // end_node = boundary;

        // val       = val << 1;

        if (end_zero - beg_zero >0)
        {
            begin_pos = beg_node + beg_zero - beg_node_zero;
            end_pos   = beg_node + end_zero - beg_node_zero;
            if(node->ZNext_)
                QuantileRangeEach(begin_pos, end_pos, i+1,beg_node ,boundary, (val<<1),ret,node->ZNext_);//
        }
        else
        {
            if (end_zero - beg_zero ==0)
            {
                //ret.push_back(val);
            }
        }
        if (end_one - beg_one >0)
        {
            begin_pos = boundary + beg_one - beg_node_one;
            end_pos   = boundary + end_one - beg_node_one;
            if(node->ONext_)
                QuantileRangeEach(begin_pos, end_pos, i+1,boundary ,end_node, (val << 1) + 1,ret,node->ONext_);//
        }
        else
        {
            if (end_one - beg_one ==0)
            {
                //ret.push_back(val);
            }
        }

    }


}

void TDTWatArray::QuantileRangeEach_NonRecursive(uint64_t begin_pos, uint64_t end_pos, size_t i,uint64_t beg_node ,uint64_t end_node, uint64_t val,vector<uint64_t>& ret) const
{
    //cout<<"QuantileRangeEach"<<"begin_pos"<<begin_pos<<"end_pos"<<end_pos<<"i"<<i<<"beg_node"<<beg_node<<"end_node"<<end_node<<"val"<<val<<"ret"<<ret.size()<<endl;
    stack<Parameter> Params;
    Params.push(Parameter(begin_pos, end_pos, i, beg_node, end_node, val));

    while(!Params.empty())
    {
        Parameter &MyParam = Params.top();
        //cout<<"QuantileRangeEach "<<" begin_pos: "<<MyParam.begin_pos<<" end_pos: "<<MyParam.end_pos<<" i: "<<MyParam.i<<" beg_node: "<<MyParam.beg_node<<" end_node: "<<MyParam.end_node<<" val: "<<MyParam.val<<" ret: "<<ret.size()<<endl;

        uint64_t param_begin_pos = MyParam.begin_pos;
        uint64_t param_end_pos = MyParam.end_pos;
        uint64_t param_val = MyParam.val;
        size_t param_i = MyParam.i;
        uint64_t param_beg_node = MyParam.beg_node;
        uint64_t param_end_node = MyParam.end_node;
        Params.pop();

        if(param_i==bit_arrays_.size())
        {
            ret.push_back(param_val);
        }
        else
        {
            const BitArray& ba = bit_arrays_[param_i];
            uint64_t beg_node_zero = ba.Rank(0, param_beg_node);
            uint64_t end_node_zero = ba.Rank(0, param_end_node);
            uint64_t beg_node_one  = param_beg_node - beg_node_zero;
            uint64_t beg_zero  = ba.Rank(0, param_begin_pos);
            uint64_t end_zero  = ba.Rank(0, param_end_pos);
            uint64_t beg_one   = param_begin_pos - beg_zero;
            uint64_t end_one   = param_end_pos - end_zero;
            uint64_t boundary  = param_beg_node + end_node_zero - beg_node_zero;

            if (end_zero - beg_zero >0)
            {
                Params.push(Parameter(param_beg_node + beg_zero - beg_node_zero,
                                      param_beg_node + end_zero - beg_node_zero,
                                      param_i+1,
                                      param_beg_node,
                                      boundary,
                                      (param_val<<1)
                                     ));
            }
            else if (end_zero - beg_zero ==0)
            {
                ret.push_back(param_val);
            }

            if (end_one - beg_one >0)
            {
                Params.push(Parameter(boundary + beg_one - beg_node_one,
                                      boundary + end_one - beg_node_one,
                                      param_i+1,
                                      boundary,
                                      param_end_node,
                                      (param_val << 1) + 1
                                     ));
            }
            else if (end_one - beg_one ==0)
            {
                ret.push_back(param_val);
            }

        }

    }

}

}
