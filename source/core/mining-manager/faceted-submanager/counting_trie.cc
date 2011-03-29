#include "counting_trie.h"

using namespace sf1r::faceted;

CountingTrie::CountingTrie():key_(1)
{
  
}

void CountingTrie::Append(const std::vector<TermIdType>& terms)
{
  for(uint32_t i=0;i<terms.size();i++)
  {
    KeyType key = 0;
    for(uint32_t j=i;j<terms.size();j++)
    {
      std::pair<KeyType,TermIdType> key_pair(key, terms[j]);
      std::pair<KeyType,ValueType>* value_pair = trie_.find(key_pair);
      if(value_pair != NULL )
      {
        key = value_pair->first;
        value_pair->second+=1;
      }
      else
      {
        key = key_;
        key_++;
        std::pair<KeyType,ValueType> insert_value(key, 1);
        trie_.insert(key_pair, insert_value);
      }
    }
  }
}

uint32_t CountingTrie::Count(const std::vector<TermIdType>& terms)
{
  std::pair<KeyType,ValueType>* value_pair = NULL;
  for(uint32_t i=0;i<terms.size();i++)
  {
    KeyType key = 0;
    std::pair<KeyType,TermIdType> key_pair(key, terms[i]);
    value_pair = trie_.find(key_pair);
    if(value_pair==NULL) return 0;
    else
    {
      key = value_pair->first;
    }
  }
  return value_pair->second;
}
