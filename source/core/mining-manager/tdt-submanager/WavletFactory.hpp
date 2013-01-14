#ifndef WAVLET_FACTORY_HPP_
#define WAVLET_FACTORY_HPP_

#include <am/succinct/wat_array/wat_array.hpp>
#include <am/succinct/wat_array/wavelet_matrix.hpp>
//#include <am/succinct/fm-index/wavelet_matrix.hpp>
//using namespace izenelib::am::succinct::fm_index;
#include <am/succinct/wat_array/bit_trie.hpp>
using namespace wat_array;
using namespace wavelet_matrix;

namespace sf1r
{

 enum WAVLETTYPE{WAT_ARRAY,WAVLET_MATRIX};
/*
class wat_array:public wat_array::WatArray
{
   public: 
     wat_array();
     ~wat_array();
};
class wavelet_matrix:public wavelet_matrix::WaveletMatrix
{
   public: 
     wavelet_matrix();
      ~wavelet_matrix();
};
*/
class WavletTree    
{    


public:    
    virtual void Init(const std::vector<uint64_t>& array){};
    virtual uint64_t Freq(uint64_t c) const{return -1;};
    virtual uint64_t Rank(uint64_t c, uint64_t pos) const{return -1;};
    virtual uint64_t Lookup(uint64_t pos) const{return -1;};
    virtual uint64_t Select(uint64_t c, uint64_t rank) const{return -1;};
    //virtual void QuantileRangeEach(uint64_t begin_pos, uint64_t end_pos, size_t i, uint64_t val,int k,vector<uint64_t>& ret,BitNode* node) const;
    virtual void QuantileRangeAll(uint64_t begin_pos,uint64_t end_pos, vector<uint64_t>& ret,const BitTrie& filter) const{};
    WavletTree(){};
    ~WavletTree(){};
};    

class WavletTree1: public WavletTree 
{    
     wat_array::WatArray wa_;
public:    
     WavletTree1(){};
     ~WavletTree1(){};
     void Init(const std::vector<uint64_t>& array){ wa_.Init(array);};
     uint64_t Freq(uint64_t c) const{return wa_.Freq(c);};
     uint64_t Rank(uint64_t c, uint64_t pos) const{return wa_.Rank(c,pos);};
     uint64_t Lookup(uint64_t pos) const{return wa_.Lookup(pos);};
     uint64_t Select(uint64_t c, uint64_t rank) const{return wa_.Select(c,rank);};
     void QuantileRangeAll(uint64_t begin_pos,uint64_t end_pos, vector<uint64_t>& ret,const BitTrie& filter) const{ wa_.QuantileRangeAll(begin_pos,end_pos,ret,filter);};
};    

class WavletTree2: public WavletTree
{    
     wavelet_matrix::WaveletMatrix wa_;
public: 
     WavletTree2(){};
     ~WavletTree2(){};   
     void Init(const std::vector<uint64_t>& array){ wa_.Init(array);};
     uint64_t Freq(uint64_t c) const{return wa_.Freq(c);};
     uint64_t Rank(uint64_t c, uint64_t pos) const{return wa_.Rank(c,pos);};
     uint64_t Lookup(uint64_t pos) const{return wa_.Lookup(pos);};
     uint64_t Select(uint64_t c, uint64_t rank) const{return wa_.Select(c,rank);};
     void QuantileRangeAll(uint64_t begin_pos,uint64_t end_pos, vector<uint64_t>& ret,const BitTrie& filter) const{ wa_.QuantileRangeAll(begin_pos,end_pos,ret,filter);};
};    

class WavletFactory    
{    

public:  
    WavletFactory(){};
    ~WavletFactory(){};   
    WavletTree* CreateWavletTree(enum WAVLETTYPE wavlettype)    
    {    
        if(wavlettype == WAT_ARRAY)   
          return new WavletTree1();    
        else if(wavlettype == WAVLET_MATRIX)    
          return new WavletTree2();   
        else 
          return NULL;    
    }    
};  


}
#endif //_
