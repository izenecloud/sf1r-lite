/**
   @file value_shared_table.cpp
   @author Kevin Hu
   @date 2010.03.22
*/


#include "value_shared_table.h"

namespace sf1r
{

bool ValueSharedTable::put(const std::string& key_str, const std::string& value)
{
    init_entries_();

    KEY_TYPE key = izenelib::util::HashFunction<std::string>::generateHash32(key_str);
    uint32_t idx = key & ENTRY_MASK;
    if (!key_ent_.at(idx))
    {
        key_ent_[idx] = new keys_t();
        addr_ent_[idx] = new addrs_t();
    }

    for (uint32_t i = 0; i<key_ent_.at(idx)->length(); ++i)
        if (key_ent_.at(idx)->at(i) == key)
            return false;

    count_++;
    key_ent_[idx]->push_back(key);

    uint64_t hv = izenelib::util::HashFunction<std::string>::generateHash64(value);
    if (last_value_ == hv)
    {
        addr_ent_[idx]->push_back(last_addr_);
        return true;
    }

    last_value_ = hv;
    last_addr_ = push2buffer_(value);
    addr_ent_[idx]->push_back(last_addr_);

    return true;
}

bool ValueSharedTable::get(const std::string& key_str, std::string& value)
{
    value = "";
    ready4query();

    if (key_ent_.length()==0)
        return false;

    KEY_TYPE key = izenelib::util::HashFunction<std::string>::generateHash32(key_str);
    uint32_t idx = key & ENTRY_MASK;

    if (!key_ent_.at(idx))
        return false;

    assert( key_ent_.at(idx)->length() == addr_ent_.at(idx)->length());
    for (uint32_t i=0; i<key_ent_.at(idx)->length(); ++i)
    {
        if (key_ent_.at(idx)->at(i) != key)
            continue;

        uint64_t addr = addr_ent_.at(idx)->at(i);
        if (addr+sizeof(uint32_t) < bs_)
        {
            uint32_t len = *(uint32_t*)(buffer_+addr);
            if (addr+sizeof(uint32_t)+len <= bs_)
            {
                value.assign((char*)(buffer_+addr+sizeof(uint32_t)), len);
                return true;
            }
        }

        boost::mutex::scoped_lock lock(lkup_mtx_);
        fseek(f_, addr, SEEK_SET);
        uint32_t len = 0;
        THROW(fread(&len, sizeof(len), 1, f_)==1, "file read error");
        char* b = new char[len];
        THROW(fread(b, len, 1, f_)==1, "file read error");
        value.assign(b, len);
        delete[] b;
        return true;
    }

    return false;
}

}
