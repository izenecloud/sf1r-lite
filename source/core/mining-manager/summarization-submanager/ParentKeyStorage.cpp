#include "ParentKeyStorage.h"

#ifdef USE_LOG_SERVER
#include <common/Utilities.h>
#include <log-manager/LogServerRequest.h>
#include <log-manager/LogServerConnection.h>
#else
#include "CommentCacheStorage.h"
#endif

#include <iostream>

namespace
{
static const std::string p2c_path("/parent_to_children");
static const std::string c2p_path("/child_to_parent");
}

namespace sf1r
{

ParentKeyStorage::ParentKeyStorage(
        const std::string& db_dir,
        CommentCacheStorage* comment_cache_storage,
        unsigned bufferSize)
    : comment_cache_storage_(comment_cache_storage)
#ifndef USE_LOG_SERVER
    , parent_to_children_db_(db_dir + p2c_path)
    , child_to_parent_db_(db_dir + c2p_path)
    , buffer_capacity_(bufferSize)
    , buffer_size_(0)
#endif
{
#ifndef USE_LOG_SERVER
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
#endif
}

ParentKeyStorage::~ParentKeyStorage()
{
}

#ifndef USE_LOG_SERVER
void ParentKeyStorage::Insert(const ParentKeyType& parent, const ChildKeyType& child)
{
    child_to_parent_db_.insert(child, parent);
    buffer_db_[parent].first.push_back(child);

    ++buffer_size_;
    if (IsBufferFull_())
        Flush();
}

void ParentKeyStorage::Update(const ParentKeyType& parent, const ChildKeyType& child)
{
    ParentKeyType old_parent;
    child_to_parent_db_.get(child, old_parent);
    if (old_parent == parent) return;
    child_to_parent_db_.update(child, parent);
    buffer_db_[parent].first.push_back(child);
    buffer_db_[old_parent].second.push_back(child);

    buffer_size_ += 2;
    if (IsBufferFull_())
        Flush();
}

void ParentKeyStorage::Delete(const ParentKeyType& parent, const ChildKeyType& child)
{
    ParentKeyType old_parent;
    if (!child_to_parent_db_.get(child, old_parent)) return;
    if (!parent.empty() && old_parent != parent) return;
    child_to_parent_db_.del(child);
    buffer_db_[old_parent].second.push_back(child);

    ++buffer_size_;
    if (IsBufferFull_())
        Flush();
}

void ParentKeyStorage::Flush()
{
    if (buffer_db_.empty()) return;

    bool dirty = false;
    for (BufferType::iterator it = buffer_db_.begin();
            it != buffer_db_.end(); ++it)
    {
        std::vector<ChildKeyType> value;
        parent_to_children_db_.get(it->first, value);

        if (!it->second.second.empty())
        {
            dirty = true;
            comment_cache_storage_->Delete(it->first);
            std::sort(it->second.second.begin(), it->second.second.end());

            std::vector<ChildKeyType> new_value;
            new_value.reserve(value.size() - it->second.second.size());

            std::vector<ChildKeyType>::iterator vit0 = value.begin();
            std::vector<ChildKeyType>::iterator vit1 = it->second.second.begin();
            while (vit1 != it->second.second.end())
            {
                if (*vit0 < *vit1)
                {
                    new_value.push_back(*vit0);
                }
                else
                {
                    ++vit1;
                }
                ++vit0;
            }
            new_value.insert(new_value.end(), vit0, value.end());

            value.swap(new_value);
        }

        if (!it->second.first.empty())
        {
            std::sort(it->second.first.begin(), it->second.first.end());

            std::vector<ChildKeyType> new_value;
            new_value.reserve(value.size() + it->second.first.size());

            std::vector<ChildKeyType>::iterator vit0 = value.begin();
            std::vector<ChildKeyType>::iterator vit1 = it->second.first.begin();
            while (vit0 != value.end() && vit1 != it->second.first.end())
            {
                if (*vit0 < *vit1)
                {
                    new_value.push_back(*vit0);
                    ++vit0;
                }
                else
                {
                    new_value.push_back(*vit1);
                    ++vit1;
                }
            }
            new_value.insert(new_value.end(), vit0, value.end());
            new_value.insert(new_value.end(), vit1, it->second.first.end());

            value.swap(new_value);
        }

        if (value.empty())
            parent_to_children_db_.del(it->first);
        else
            parent_to_children_db_.update(it->first, value);
    }

    if (dirty) comment_cache_storage_->Flush();
    child_to_parent_db_.flush();
    parent_to_children_db_.flush();
    buffer_db_.clear();
    buffer_size_ = 0;
}
#endif

bool ParentKeyStorage::GetChildren(const ParentKeyType& parent, std::vector<ChildKeyType>& children)
{
#ifdef USE_LOG_SERVER
    LogServerConnection& conn = LogServerConnection::instance();
    GetDocidListRequest docidListReq;
    UUID2DocidList docidListResp;

    docidListReq.param_.uuid_ = parent;

    conn.syncRequest(docidListReq, docidListResp);
    if (docidListReq.param_.uuid_ != docidListResp.uuid_)
        return false;

    children.swap(docidListResp.docidList_);
    return true;
#else
    boost::shared_lock<boost::shared_mutex> lock(mutex_);
    return parent_to_children_db_.get(parent, children);
#endif
}

bool ParentKeyStorage::GetParent(const ChildKeyType& child, ParentKeyType& parent)
{
#ifdef USE_LOG_SERVER
    LogServerConnection& conn = LogServerConnection::instance();
    GetUUIDRequest uuidReq;
    Docid2UUID uuidResp;

    uuidReq.param_.docid_ = child;

    conn.syncRequest(uuidReq, uuidResp);
    if (uuidReq.param_.docid_ != uuidResp.docid_)
        return false;

    parent = uuidResp.uuid_;
    return true;
#else
    boost::shared_lock<boost::shared_mutex> lock(mutex_);
    return child_to_parent_db_.get(child, parent);
#endif
}

}
