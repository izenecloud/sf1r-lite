#include "ParentKeyStorage.h"

#include <iostream>

namespace sf1r
{

ParentKeyStorage::ParentKeyStorage(
        const std::string& dbPath,
        unsigned bufferSize)
    : parent_key_db_(dbPath)
    , buffer_capacity_(bufferSize)
    , delimit_(",", UString::UTF_8)
{
}

ParentKeyStorage::~ParentKeyStorage()
{
}

void ParentKeyStorage::AppendUpdate(const UString& key, const UString& value)
{
    buffer_db_.insert(key, value);

    if (IsBufferFull_())
        Flush();
}

void ParentKeyStorage::Flush()
{
    BufferType::iterator it = buffer_db_.begin();
    for (; it != buffer_db_.end(); ++it)
    {
        UString v;
        if (parent_key_db_.get(it->first,v))
        {
            v.append(delimit_);
            v.append(it->second);
            parent_key_db_.update(it->first, v);
        }
        else
        {
            parent_key_db_.insert(it->first,it->second);
        }
    }
    parent_key_db_.flush();
    buffer_db_.clear();
}

}
