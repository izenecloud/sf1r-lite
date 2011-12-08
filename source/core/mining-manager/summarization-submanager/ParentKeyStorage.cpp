#include "ParentKeyStorage.h"

#include <iostream>

namespace sf1r
{

const std::string ParentKeyStorage::p2c_path("/parent_to_children");
const std::string ParentKeyStorage::c2p_path("/child_to_parent");

ParentKeyStorage::ParentKeyStorage(
        const std::string& db_dir,
        unsigned bufferSize)
    : parent_to_children_db_(db_dir + p2c_path)
    , child_to_parent_db_(db_dir + c2p_path)
    , buffer_capacity_(bufferSize)
    , buffer_size_(0)
{
    if (!parent_to_children_db_.open())
    {
        boost::filesystem::remove_all(db_dir + p2c_path);
        parent_to_children_db_.open();
    }
    if (!child_to_parent_db_.open())
    {
        boost::filesystem::remove_all(db_dir + c2p_path);
        child_to_parent_db_.open();
    }
}

ParentKeyStorage::~ParentKeyStorage()
{
}

void ParentKeyStorage::AppendUpdate(const UString& parent, const UString& child)
{
    child_to_parent_db_.update(child, parent);
    buffer_db_[parent].push_back(child);

    ++buffer_size_;
    if (IsBufferFull_())
        Flush();
}

void ParentKeyStorage::Flush()
{
    child_to_parent_db_.flush();
    if (buffer_db_.empty()) return;

    BufferType::iterator it = buffer_db_.begin();
    for (; it != buffer_db_.end(); ++it)
    {
        std::vector<UString> v;
        if (parent_to_children_db_.get(it->first, v))
        {
//          for (std::vector<UString>::iterator vit = it->second.begin();
//                  vit != it->second.end(); ++vit)
//          {
//              v.push_back(UString());
//              v.back().swap(*vit);
//          }
            v.insert(v.end(), it->second.begin(), it->second.end());
            parent_to_children_db_.update(it->first, v);
        }
        else
        {
            parent_to_children_db_.update(it->first, it->second);
        }
    }

    parent_to_children_db_.flush();
    buffer_db_.clear();
    buffer_size_ = 0;
}

bool ParentKeyStorage::GetChildren(const UString& parent, std::vector<UString>& children)
{
    boost::shared_lock<boost::shared_mutex> lock(mutex_);
    return parent_to_children_db_.get(parent, children);
}

bool ParentKeyStorage::GetParent(const UString& child, UString& parent)
{
    boost::shared_lock<boost::shared_mutex> lock(mutex_);
    return child_to_parent_db_.get(child, parent);
}

}
