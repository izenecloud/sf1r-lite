#include "ParentKeyStorage.h"

#include <iostream>

namespace sf1r
{

ParentKeyStorage::ParentKeyStorage(
        const std::string& dbPath,
        unsigned bufferSize)
    : parent_key_db_(dbPath)
    , buffer_capacity_(bufferSize)
    , buffer_size_(0)
    , delimit_(",", UString::UTF_8)
{
}

ParentKeyStorage::~ParentKeyStorage()
{
}

void ParentKeyStorage::AppendUpdate(const UString& key, const UString& value)
{
    std::vector<UString>& v = buffer_db_[key];
    v.push_back(value);

    ++buffer_size_;
    if (IsBufferFull_())
        Flush();
}

void ParentKeyStorage::Flush()
{
    BufferType::iterator it = buffer_db_.begin();
    for (; it != buffer_db_.end(); ++it)
    {
        std::vector<UString> v;
        parent_key_db_.get(it->first, v);
        v.insert(v.end(), it->second.begin(), it->second.end());
        parent_key_db_.update(it->first, v);
    }
    parent_key_db_.flush();
    buffer_db_.clear();
    buffer_size_ = 0;
}

bool ParentKeyStorage::Get(const UString& key, std::vector<UString>& results)
{
    boost::shared_lock<boost::shared_mutex> lock(mutex_);
    return parent_key_db_.get(key, results);
}

}
