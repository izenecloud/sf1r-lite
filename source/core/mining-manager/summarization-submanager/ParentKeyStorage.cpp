#include "ParentKeyStorage.h"

#include <util/string/StringUtils.h>

#include <iostream>

namespace sf1r
{

ParentKeyStorage::ParentKeyStorage(
    const std::string& dbPath,
    unsigned bufferSize)
    :parent_key_db_(dbPath)
    ,buffer_capacity_(bufferSize)
    ,buffer_size_(0)
    ,delimit_(",",UString::UTF_8)
{
}

ParentKeyStorage::~ParentKeyStorage()
{
}

void ParentKeyStorage::AppendUpdate(
    const UString& key, 
    const UString& value)
{
    if(IsBufferFull_())
    {
        FlushBuffer_();
    }
    BufferType::iterator it = buffer_db_.find(key);
    if(it == buffer_db_.end())
        it == buffer_db_.insert(std::make_pair(key,value)).first;
    else
    {
        it->second.append(delimit_);
        it->second.append(value);
    }
    ++buffer_size_;
}

void ParentKeyStorage::Flush()
{
    FlushBuffer_();
    parent_key_db_.flush();
}

bool ParentKeyStorage::Get(
    const UString& key, 
    std::list<UString>& results)
{
    boost::shared_lock<boost::shared_mutex> lock(mutex_);
    UString value;
    if(!parent_key_db_.get(key,value))
        return false;

    izenelib::util::Split<UString, std::list<UString> >(value,results,delimit_);
    return true;
}

void ParentKeyStorage::FlushBuffer_()
{
    boost::lock_guard<boost::shared_mutex> lock(mutex_);
    BufferType::iterator it = buffer_db_.begin();
    for(; it != buffer_db_.end(); ++it)
    {
        UString v;
        if(parent_key_db_.get(it->first,v))
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
    buffer_db_.clear();
    buffer_size_ = 0;
}

}
