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
    , mode_(ParentKeyStorage::NONE)
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

void ParentKeyStorage::Insert(const UString& parent, const UString& child)
{
    if (mode_ != INSERT) Flush();
    mode_ = INSERT;
    child_to_parent_db_.insert(child, parent);
    buffer_db_[parent].first.push_back(child);

    ++buffer_size_;
    if (IsBufferFull_())
        Flush();
}

void ParentKeyStorage::Update(const UString& parent, const UString& child)
{
    if (mode_ != UPDATE) Flush();
    mode_ = UPDATE;
    UString old_parent;
    child_to_parent_db_.get(child, old_parent);
    if (old_parent == parent) return;
    child_to_parent_db_.update(child, parent);
    buffer_db_[parent].first.push_back(child);
    buffer_db_[old_parent].second.push_back(child);

    buffer_size_ += 2;
    if (IsBufferFull_())
        Flush();
}

void ParentKeyStorage::Delete(const UString& parent, const UString& child)
{
    if (mode_ != DELETE) Flush();
    mode_ = DELETE;
    UString old_parent;
    child_to_parent_db_.get(child, old_parent);
    if (!parent.empty() && old_parent != parent) return;
    child_to_parent_db_.del(child);
    buffer_db_[old_parent].second.push_back(child);

    ++buffer_size_;
    if (IsBufferFull_())
        Flush();
}

void ParentKeyStorage::Flush()
{
    if (mode_ == NONE || buffer_db_.empty()) return;

    for (BufferType::iterator it = buffer_db_.begin();
            it != buffer_db_.end(); ++it)
    {
        std::vector<UString> v;
        parent_to_children_db_.get(it->first, v);
        for (std::vector<UString>::const_iterator cit = it->second.second.begin();
                cit != it->second.second.end(); ++cit)
        {
            for (std::vector<UString>::iterator vit = v.begin();
                    vit != v.end(); ++vit)
            {
                if (*vit == *cit)
                {
                    v.erase(vit);
                    break;
                }
            }
        }
        v.reserve(v.size() + it->second.first.size());
        for (std::vector<UString>::iterator vit = it->second.first.begin();
                vit != it->second.first.end(); ++vit)
        {
            v.push_back(UString());
            v.back().swap(*vit);
        }
        parent_to_children_db_.update(it->first, v);
    }

    child_to_parent_db_.flush();
    parent_to_children_db_.flush();
    buffer_db_.clear();
    buffer_size_ = 0;
    mode_ = NONE;
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
