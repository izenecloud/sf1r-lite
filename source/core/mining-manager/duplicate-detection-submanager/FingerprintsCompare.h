
#ifndef FINGERPRINTSCOMPARE_H_
#define FINGERPRINTSCOMPARE_H_


#include "DupTypes.h"
#include <string>
#include <vector>
#include <ir/dup_det/index_fp.hpp>
#include <ir/dup_det/integer_dyn_array.hpp>
#include "charikar_algo.h"
#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <am/sequence_file/SimpleSequenceFile.hpp>

namespace sf1r{

template<uint8_t FP_SIZE=64, uint8_t K=3, uint8_t GROUP_COUNT=4>
class FingerprintsCompare
{
 public:
  FingerprintsCompare():num_(0), nStartCount_(0), invalid_(true)
  {
    //nouse
  }
  
  FingerprintsCompare(uint8_t num):num_(num), nStartCount_(0), invalid_(true)
  {
    init_(num_);
  }
  
  ~FingerprintsCompare()
  {
//     if( !invalid_)
//       free(nStartCount_);
  }
  
  static uint8_t GetSortCount()
  {
    uint32_t a = 1;
    uint32_t b = 1;
    for(uint8_t i=1;i<=K;++i)
    {
      a *= (GROUP_COUNT-i+1);
      b *= i;
    }
    return a/b;
  }
  
  static uint8_t GetFPSize()
  {
    return FP_SIZE;
  }
  
  static uint8_t GetKValue()
  {
    return K;
  }
  
  bool operator() (const std::pair<uint32_t, izenelib::util::CBitArray>& left, const std::pair<uint32_t, izenelib::util::CBitArray>& right) 
  { 
    if( invalid_ ) return 1;
    uint64_t int_left = GetBitsValue( left.second);
    uint64_t int_right = GetBitsValue( right.second);
    return int_left < int_right;
  }
  
  inline uint64_t GetBitsValue(const izenelib::util::CBitArray& bitArray)
  {
    if( invalid_ ) return 0;
    const uint8_t* p = bitArray.GetBuffer();
//     std::cout<<"[WWWW]"<<","<<nStartCount_[0]<<","<<nStartCount_[1]<<std::endl;
    return izenelib::util::CBitArray::GetBitsValue<uint64_t>(p, nStartCount_);
  }
  
 private:
  void init_(uint8_t num)
  {
    std::vector<uint8_t> blocks(GROUP_COUNT, FP_SIZE/GROUP_COUNT);
    uint8_t remain = FP_SIZE % GROUP_COUNT;
    for(uint8_t i=0;i<remain;i++)
    {
      blocks[i] += 1;
    }
    std::vector<std::pair<int, int> > blocks_pos(blocks.size() );
    blocks_pos[0].first = 0;
    blocks_pos[0].second = (int)blocks[0];
    for(uint8_t i=1;i<blocks_pos.size();++i)
    {
      blocks_pos[i].first = blocks_pos[i-1].first + blocks_pos[i-1].second;
      blocks_pos[i].second = (int)blocks[i];
    }
    //select num th block in C_GC^(GC-K) in blocks
    uint8_t size_ = GROUP_COUNT - K;
//     nStartCount_ = (int*) malloc(sizeof(int)*size_*2);
    //init
    
    std::vector<uint8_t > vec(size_);
    for( uint8_t i=0;i<size_;i++)
    {
      vec[i] = i;
    }
    
    uint8_t index = 0;
    while( true )
    {
      if( index == num ) break;
      //set the value
      transform_( vec, size_-1, blocks_pos.size() );
      ++index;
    }
    
//     std::cout<<"[DUPD-GROUP] ("<<(int)num<<") ";
//     for( uint8_t i=0;i<size_;i++)
//     {
//       std::cout<<(int)vec[i]<<",";
//     }
//     std::cout<<std::endl;
    
    for( uint8_t i=0;i<size_;i++)
    {
//       nStartCount_[i*2] = blocks_pos[vec[i] ].first;
//       nStartCount_[i*2+1] = blocks_pos[vec[i] ].second;
//       std::cout<<"nStartCount_ "<<(int)i<<","<<nStartCount_[i*2]<<","<<nStartCount_[i*2+1]<<std::endl;
      
      nStartCount_.push_back( blocks_pos[vec[i]] );
    }
    invalid_ = false;
  }
  
  void transform_(std::vector<uint8_t >& vec, uint32_t index, uint32_t size)
  {
    uint8_t pos = vec.size()-index;
    if( vec[index] == size-pos )
    {
      transform_(vec, index-1, size);
      return;
    }
    vec[index]++;
    for(uint32_t i=index+1; i<vec.size();i++ )
    {
      vec[i] = vec[index] + i-index;
    }
  }

  
 private:
  uint8_t num_;
  uint8_t sort_count_;
  std::vector<std::pair<int, int> > nStartCount_;
//   int* nStartCount_;
//   int size_;
  bool invalid_;
};

}

#endif /* DUPDETECTOR_H_ */
