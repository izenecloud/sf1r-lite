/**
   @file value_shared.h
   @author Kevin Hu
   @date 2010.03.22
*/
#ifndef _VALUE_SHARED_H_
#define _VALUE_SHARED_H_

#include <types.h>
#include <am/graph_index/dyn_array.hpp>
#include <boost/thread/mutex.hpp>
#include <util/hashFunction.h>
#include <stdio.h>
#include <string>
namespace sf1r
{

/**
 * @class ValueSharedTable
 **/
class ValueSharedTable
{
    enum { ENTRY_POW = 17};
    enum {ENTRY_SIZE = (2<<ENTRY_POW)};
    enum {ENTRY_MASK = ENTRY_SIZE-1};

    typedef uint32_t KEY_TYPE;
    typedef izenelib::am::DynArray<KEY_TYPE> keys_t;
    typedef izenelib::am::DynArray<uint64_t> addrs_t;

    typedef izenelib::am::DynArray<keys_t*> key_entry_t;
    typedef izenelib::am::DynArray<addrs_t*> addr_entry_t;

    std::string filenm_;
    FILE* f_;
    uint64_t count_;
    uint8_t* buffer_;
    uint32_t bs_;
    uint32_t buf_pos_;
    uint64_t last_value_;
    uint64_t last_addr_;
    bool     ready4query_;

    key_entry_t key_ent_;
    addr_entry_t addr_ent_;

    boost::mutex lkup_mtx_;

    inline void enlarge_(uint32_t s)
    {
        if (!buffer_)
        {
            buffer_ = (uint8_t*)malloc(bs_);
            return;
        }

        if (s < bs_)
        {
            buf_pos_ = 0;
            return;
        }

        bs_ = s;
        buffer_ = (uint8_t*)realloc(buffer_, bs_);
        buf_pos_ = 0;

        return;
    }

    inline void init_entries_()
    {
        if (key_ent_.length()==0)
        {
            key_ent_.reserve(ENTRY_SIZE);
            addr_ent_.reserve(ENTRY_SIZE);

            for (uint32_t i=0; i<ENTRY_SIZE; ++i)
            {
                key_ent_.push_back(NULL);
                addr_ent_.push_back(NULL);
            }
        }
    }

    inline void clear_()
    {
        if ( 0 == key_ent_.length() )
            return ;

        count_ = 0;

        for (uint32_t i=0; i<ENTRY_SIZE; ++i)
        {
            if ( key_ent_[i] == NULL )
                continue;

            assert(addr_ent_.at(i));
            delete key_ent_[i];
            delete addr_ent_[i];

            key_ent_[i] = NULL;
            addr_ent_[i] = NULL;
        }
    }

    inline void save_()
    {
        FILE* f = fopen((filenm_+".tbl").c_str(), "w+");
        THROW(f, "fail to create file");
        if (!f)
            return;

        THROW(fwrite(&count_, sizeof(count_), 1, f)==1, "file write error");
        uint32_t i=0;
        for (; i<ENTRY_SIZE; ++i)
        {
            if (!key_ent_.at(i))
                continue;

            THROW(fwrite(&i, sizeof(i), 1, f)==1, "file write error");
            key_ent_.at(i)->save(f);
            addr_ent_.at(i)->save(f);
            assert( key_ent_.at(i)->length() == addr_ent_.at(i)->length());
        }

        i = -1;
        THROW(fwrite(&i, sizeof(i), 1, f)==1, "file write error");
        fclose(f);
    }

    inline void load_()
    {
        //open value file
        if (!f_)
        {
            f_ = fopen((filenm_+".val").c_str(), "r+");
            if (!f_)
                f_ = fopen((filenm_+".val").c_str(), "w+");
            if (!f_)
            {
                throw "[ERROR]: unable to create file.";
                return;
            }
        }

        //loading table
        init_entries_();
        FILE* f = fopen((filenm_+".tbl").c_str(), "r");
        if (!f)
            return;

        THROW(fread(&count_, sizeof(count_), 1, f)==1, "file read error");
        uint32_t idx;
        THROW(fread(&idx, sizeof(idx), 1, f)==1, "file read error");
        while (idx!=(uint32_t)-1)
        {
            if (key_ent_[idx])
            {
                assert(addr_ent_[idx]);
                delete addr_ent_[idx];
                delete key_ent_[idx];
            }

            key_ent_[idx] = new keys_t();
            addr_ent_[idx] = new addrs_t();
            key_ent_[idx]->load(f);
            addr_ent_[idx]->load(f);
            assert( key_ent_.at(idx)->length() == addr_ent_.at(idx)->length());
            THROW(fread(&idx, sizeof(idx), 1, f)==1, "file read error");
        }

        fclose(f);
    }

    inline uint64_t push2buffer_(const std::string& str)
    {
        fseek(f_, 0, SEEK_END);
        if (!buffer_)
            buffer_ = (uint8_t*)malloc(bs_);

        if (buf_pos_ + sizeof(uint32_t) + str.length() > bs_)
        {
            THROW(fwrite(buffer_, buf_pos_, 1, f_)==1, "file write error");
            buf_pos_ = 0;
        }

        uint64_t addr = ftell(f_)+ buf_pos_;
        *(uint32_t*)(buffer_+buf_pos_) = str.length();
        buf_pos_ += sizeof(uint32_t);
        memcpy(buffer_+buf_pos_, str.c_str(), str.length());
        buf_pos_ += str.length();
        return addr;
    }

public:
    /**
       @brief a constructor
    */
    ValueSharedTable(const char* filenm, uint32_t buffer_size = 100000000)
            :filenm_(filenm), f_(NULL), count_(0), buffer_(NULL),
            bs_(buffer_size), buf_pos_(0), last_value_(-1),
            last_addr_(-1), ready4query_(false)
    {
        load_();
    }

    ~ValueSharedTable()
    {
        clear_();
        if (buffer_)
            free(buffer_);
        if (f_)
            fclose(f_);
    }

    void flush()
    {
        save_();
        if (buf_pos_)
        {
            THROW(fwrite(buffer_, buf_pos_, 1, f_)==1, "file write error");
            buf_pos_ = 0;
        }

        fflush(f_);
    }

    inline void ready4query()
    {
        boost::mutex::scoped_lock lock(lkup_mtx_);

        if (ready4query_)
            return ;

        enlarge_(bs_);
        fseek(f_, 0, SEEK_SET);
        fread(buffer_, bs_, 1, f_);
        ready4query_ = true;
    }

    bool put(const std::string& key, const std::string& value);

    bool get(const std::string& key, std::string& value);

    inline uint64_t item_num()const
    {
        return count_;
    }

}
;

}


#endif
