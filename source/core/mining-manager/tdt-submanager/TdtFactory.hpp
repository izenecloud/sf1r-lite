#ifndef WAVLET_FACTORY_HPP_
#define WAVLET_FACTORY_HPP_

#include "wavelet-tree/wat_array.hpp"
#include "wavelet-tree/wavelet_matrix.hpp"
#include <iostream>
using namespace std;
namespace sf1r
{

enum TDTTYPE{WAT_ARRAY,WAVLET_MATRIX,NON_WAVLET};

class TdtMemory    
{    
public:    
    TdtMemory(){}
    virtual ~TdtMemory(){}
    virtual void Init(const std::vector<uint64_t>& array) = 0;
    virtual uint64_t Freq(uint64_t c) const = 0;
    //virtual uint64_t Rank(uint64_t c, uint64_t pos) const = 0;
    //virtual uint64_t Lookup(uint64_t pos) const = 0;
    virtual uint64_t Select(uint64_t c, uint64_t rank) const = 0;
    virtual void Clear()  = 0;
    virtual void QuantileRangeAll(uint64_t begin_pos,uint64_t end_pos, vector<uint64_t>& ret,const BitTrie& filter) const = 0;
    virtual void Save(ostream& os) const = 0;
    virtual void Load(istream& is) = 0;

};    

class WavletTree1: public TdtMemory 
{    
     TDTWatArray wa_;
public:    
     WavletTree1(){}
     ~WavletTree1(){}
     void Init(const std::vector<uint64_t>& array){ wa_.Init(array);}
     uint64_t Freq(uint64_t c) const{return wa_.Freq(c);}
     //uint64_t Rank(uint64_t c, uint64_t pos) const{return wa_.Rank(c,pos);}
     //uint64_t Lookup(uint64_t pos) const{return wa_.Lookup(pos);}
     uint64_t Select(uint64_t c, uint64_t rank) const{return wa_.Select(c,rank);}
     void Clear() { wa_.Clear();};
     void QuantileRangeAll(uint64_t begin_pos,uint64_t end_pos, vector<uint64_t>& ret,const BitTrie& filter) const{ wa_.QuantileRangeAll(begin_pos,end_pos,ret,filter);}
     void Save(ostream& os) const { wa_.Save(os);};
     void Load(istream& is) { wa_.Load(is);};
};    

class WavletTree2: public TdtMemory
{    
     TDTWaveletMatrix wa_;
public: 
     WavletTree2(){};
     ~WavletTree2(){};   
     void Init(const std::vector<uint64_t>& array){ wa_.Init(array);};
     uint64_t Freq(uint64_t c) const{return wa_.Freq(c);};
     //uint64_t Rank(uint64_t c, uint64_t pos) const{return wa_.Rank(c,pos);};
     //uint64_t Lookup(uint64_t pos) const{return wa_.Lookup(pos);};
     uint64_t Select(uint64_t c, uint64_t rank) const{return wa_.Select(c,rank);};
     void Clear() { wa_.Clear();};
     void QuantileRangeAll(uint64_t begin_pos,uint64_t end_pos, vector<uint64_t>& ret,const BitTrie& filter) const{ wa_.QuantileRangeAll(begin_pos,end_pos,ret,filter);};
     void Save(ostream& os) const { wa_.Save(os);};
     void Load(istream& is) { wa_.Load(is);};
};    

class NonWavletTree: public TdtMemory
{    
     std::vector<std::vector<int> > linkout_;
     std::vector<uint64_t> array_;
public: 
     NonWavletTree(){};
     ~NonWavletTree(){};   
     void Init(const std::vector<uint64_t>& array)
     { 
           array_=array;
           linkout_.resize(2740000);
           for(unsigned i=0;i<array.size();i++)
           {
              //if(i%100000==0){cout<<"have build link"<<i<<endl;}
              uint64_t j=array[i];
              // cout<<"i"<<i<<"j"<<j<<endl;
              if(j>linkout_.size())
              {linkout_.resize(j+1);}
              linkout_[j].push_back(i);
           }
     };
     uint64_t Freq(uint64_t c) const{return linkout_[c].size();};
     //uint64_t Rank(uint64_t c, uint64_t pos) const{return -1;};
     //uint64_t Lookup(uint64_t pos) const{return -1;};
     uint64_t Select(uint64_t c, uint64_t rank) const{return linkout_[c][rank-1]+1;};
     void Clear() { linkout_.clear();};
     void QuantileRangeAll(uint64_t begin_pos,uint64_t end_pos, vector<uint64_t>& ret,const BitTrie& filter) const
     {  
        for (unsigned i=begin_pos;i<=end_pos;i++)
        {
            if(filter.exist(array_[i]))
            {
                ret.push_back(array_[i]);
            }

        }
     };
     void Save(ostream& os) const
     {
          size_t linkoutsize=linkout_.size();
          os.write((const char*)&linkoutsize, sizeof(linkoutsize));
          for (size_t i = 0; i <linkoutsize; ++i)
          {
              size_t linkoutisize=linkout_[i].size();
              os.write((const char*)&linkoutisize,sizeof(linkoutisize));
              os.write((const char*)&linkout_[i][0],sizeof(linkout_[i][0])*linkoutisize);
              
          }
          size_t arraysize=array_.size();
          os.write((const char*)&arraysize, sizeof(arraysize));
          os.write((const char*)&array_[0],sizeof(array_[0]*arraysize));
          
     }
     void Load(istream& is) 
     {
          size_t linkoutsize;
          is.read((char*)(&linkoutsize), sizeof(linkoutsize));
          linkout_.resize(linkoutsize);
          for (size_t i = 0; i <linkout_.size(); ++i)
          {
              size_t linkoutisize=0;
              is.read((char*)&linkoutisize,sizeof(linkoutisize));
              linkout_[i].resize(linkoutisize);
              is.read((char*)(&linkout_[i][0]),sizeof(linkout_[i][0])*linkoutisize);
              
          }
          size_t arraysize;
          is.read((char*)(&arraysize), sizeof(arraysize));
          array_.resize(arraysize);
          is.read((char*)&array_[0],sizeof(array_[0])*arraysize);
          
     }
};    

class TdtFactory    
{    

public:  
    TdtFactory(){};
    ~TdtFactory(){};   
    TdtMemory* CreateTdtMemory(enum TDTTYPE wavlettype)    
    {    
        if(wavlettype == WAT_ARRAY)   
          return new WavletTree1();    
        else if(wavlettype == WAVLET_MATRIX)    
          return new WavletTree2();   
        else if(wavlettype == NON_WAVLET)    
          return new NonWavletTree();  
        else 
          return NULL;    
    }    
};  


}
#endif //_
