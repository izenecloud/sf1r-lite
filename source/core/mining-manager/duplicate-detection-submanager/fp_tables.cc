#include "fp_tables.h"

using namespace sf1r;

void FpTables::GenTables(uint32_t f, uint8_t k, uint8_t partition_num, std::vector<FpTable>& table_list)
{
  uint32_t a = 1;
  uint32_t b = 1;
  for(uint8_t i=1;i<=k;++i)
  {
    a *= (partition_num-i+1);
    b *= i;
  }
  uint32_t table_count = a/b;
  
  std::vector<uint8_t> blocks(partition_num, f/partition_num);
  uint8_t remain = f % partition_num;
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
  //select blocks in C_pn^(pn-k) in blocks
  uint8_t len = partition_num - k;
  //init
  
  std::vector<uint8_t > vec(len);
  for( uint8_t i=0;i<len;i++)
  {
    vec[i] = i;
  }
  
  uint32_t table_id = 0;
  while(true)
  {
    //add
    std::vector<std::pair<int, int> > permute;
    for( uint8_t i=0;i<len;i++)
    {
      permute.push_back( blocks_pos[vec[i]] );
    }
    FpTable table(permute);
    table_list.push_back(table);
    table_id++;
    if(table_id==table_count) break;
    //set the value
    transform_( vec, len-1, blocks_pos.size() );
  }
}

void FpTables::transform_(std::vector<uint8_t >& vec, uint32_t index, uint32_t size)
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

