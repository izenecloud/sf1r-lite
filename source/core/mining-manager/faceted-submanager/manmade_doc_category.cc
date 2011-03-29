#include "manmade_doc_category.h"
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/serialization/list.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/filesystem.hpp>
using namespace sf1r::faceted;

ManmadeDocCategory::ManmadeDocCategory(const std::string& dir):dir_(dir)
{
}
ManmadeDocCategory::~ManmadeDocCategory()
{
}
bool ManmadeDocCategory::Open()
{
  try
  {
    if( boost::filesystem::exists(dir_) )
    {
      std::ifstream ifs(dir_.c_str(), std::ios::binary);
      if( ifs.fail()) return false;
      {
        boost::archive::text_iarchive ia(ifs);
        ia >> items_ ;
      }
      ifs.close();
      if( ifs.fail() )
      {
        return false;
      }
    }
  }
  catch(std::exception& ex)
  {
    return false;
  }
  return true;
}
bool ManmadeDocCategory::Add(const std::vector<ManmadeDocCategoryItem>& sorted_items)
{
  if(sorted_items.empty()) return true;
  boost::lock_guard<boost::mutex> lock(mutex_);
  izenelib::am::rde_hash<izenelib::util::UString, uint32_t> map;
  for(uint32_t i=0;i<items_.size();i++)
  {
    map.insert(items_[i].str_docid, i);
  }
  for(uint32_t i=0;i<sorted_items.size();i++)
  {
    uint32_t find_index = 0;
    if(map.get(sorted_items[i].str_docid, find_index))
    {
      items_[find_index].cname = sorted_items[i].cname;
      items_[find_index].cid = sorted_items[i].cid;
    }
    else
    {
      items_.push_back(sorted_items[i]);
    }
  }
  std::stable_sort(items_.begin(), items_.end(), ManmadeDocCategoryItem::CompareStrDocId );
  //save
  std::ofstream ofs(dir_.c_str());
  if( ofs.fail()) return false;
  {
    boost::archive::text_oarchive oa(ofs);
    oa << items_ ;
  }
  ofs.close();
  if( ofs.fail() )
  {
    return false;
  }
  return true;
}

bool ManmadeDocCategory::Get(std::vector<ManmadeDocCategoryItem>& items)
{
  items = items_;
  return true;
}
