///
/// @file   forward_indexer.hpp
/// @brief
/// @author Kevin Hu
/// @date   2009-11-16
/// @details
/// - Log
///
#ifndef DOC_TABLE_HPP
#define DOC_TABLE_HPP
#include <string>

#include "MiningManagerDef.h"

#include <util/hashFunction.h>
#include <ctime>
#include <boost/filesystem.hpp>

#include <algorithm>
#include <am/dyn_array.hpp>
#include <time.h>
#include <boost/thread/mutex.hpp>

namespace sf1r
{
/*!
 This structure is used for store address and docid pair. It can be sorted by address.
 */
struct ADRESS_ID_PAIR
{
    char addr[8];
    char docid[4];
    char index[4];

    uint32_t& DOCID_()
    {
        return *(uint32_t*) docid;
    }

    uint32_t& INDEX_()
    {
        return *(uint32_t*) index;
    }

    uint64_t& ADDR_()
    {
        return *(uint64_t*) addr;
    }

    uint32_t DOCID() const
    {
        return *(uint32_t*) docid;
    }

    uint32_t INDEX() const
    {
        return *(uint32_t*) index;
    }

    uint64_t ADDR() const
    {
        return *(uint64_t*) addr;
    }

    inline ADRESS_ID_PAIR(uint64_t j = -1, uint32_t k = -1, uint32_t i = -1)
    {
        ADDR_() = j;
        DOCID_() = k;
        INDEX_() = i;
    }

    inline ADRESS_ID_PAIR(const ADRESS_ID_PAIR& other)
    {
        DOCID_() = other.DOCID();
        ADDR_() = other.ADDR();
        INDEX_() = other.INDEX();
    }

    inline ADRESS_ID_PAIR& operator =(const ADRESS_ID_PAIR& other)
    {
        DOCID_() = other.DOCID();
        ADDR_() = other.ADDR();
        INDEX_() = other.INDEX();
        return *this;
    }

    inline bool operator ==(const ADRESS_ID_PAIR& other) const
    {
        return (ADDR() == other.ADDR());
    }

    inline bool operator !=(const ADRESS_ID_PAIR& other) const
    {
        return (ADDR() != other.ADDR());
    }

    inline bool operator <(const ADRESS_ID_PAIR& other) const
    {
        return (ADDR() < other.ADDR());
    }

    inline bool operator >(const ADRESS_ID_PAIR& other) const
    {
        return (ADDR() > other.ADDR());
    }

    inline bool operator <=(const ADRESS_ID_PAIR& other) const
    {
        return (ADDR() <= other.ADDR());
    }

    inline bool operator >=(const ADRESS_ID_PAIR& other) const
    {
        return (ADDR() >= other.ADDR());
    }

    friend std::ostream& operator <<(std::ostream& os, const ADRESS_ID_PAIR& v)
    {
        os << "<" << v.ADDR() << "," << v.DOCID() << "," << v.INDEX() << ">";
        return os;
    }
};

/*!
 This structure is used for store docid and address pair. It can be sorted by docid.
 */
struct ID_ADRESS_PAIR
{
    char docid[4];
    char addr[8];

    uint32_t& DOCID_()
    {
        return *(uint32_t*) docid;
    }

    uint64_t& ADDR_()
    {
        return *(uint64_t*) addr;
    }

    uint32_t DOCID() const
    {
        return *(uint32_t*) docid;
    }

    uint64_t ADDR() const
    {
        return *(uint64_t*) addr;
    }

    inline ID_ADRESS_PAIR(uint32_t j, uint64_t k = -1)
    {
        DOCID_() = j;
        ADDR_() = k;
    }

    inline ID_ADRESS_PAIR(const ID_ADRESS_PAIR& other)
    {
        DOCID_() = other.DOCID();
        ADDR_() = other.ADDR();
    }

    inline ID_ADRESS_PAIR& operator =(const ID_ADRESS_PAIR& other)
    {
        DOCID_() = other.DOCID();
        ADDR_() = other.ADDR();
        return *this;
    }

    inline bool operator ==(const ID_ADRESS_PAIR& other) const
    {
        return (DOCID() == other.DOCID());
    }

    inline bool operator !=(const ID_ADRESS_PAIR& other) const
    {
        return (DOCID() != other.DOCID());
    }

    inline bool operator <(const ID_ADRESS_PAIR& other) const
    {
        return (DOCID() < other.DOCID());
    }

    inline bool operator >(const ID_ADRESS_PAIR& other) const
    {
        return (DOCID() > other.DOCID());
    }

    inline bool operator <=(const ID_ADRESS_PAIR& other) const
    {
        return (DOCID() <= other.DOCID());
    }

    inline bool operator >=(const ID_ADRESS_PAIR& other) const
    {
        return (DOCID() >= other.DOCID());
    }

    inline uint32_t operator %(uint32_t p) const
    {
        return (DOCID() % p);
    }

    friend std::ostream& operator <<(std::ostream& os, const ID_ADRESS_PAIR& v)
    {
        os << "<" << v.DOCID() << "," << v.ADDR() << ">";
        return os;
    }
};

/**
 @class ForwardIndexer
 @brief a class used for storing label collections like a forward indexer.
 */
template<uint32_t CACHE_SIZE = 550000000,//!< cache size for accessing label collection
        uint32_t ENTRY_SIZE = 1000000 //!< entry size for hash map
>
class DocTable
{
    typedef izenelib::am::DynArray<struct ID_ADRESS_PAIR> slot_t;
    typedef izenelib::am::DynArray<slot_t*> entry_t;
    typedef izenelib::am::DynArray<struct ADRESS_ID_PAIR> addr_id_pairs_t;
    typedef izenelib::am::DynArray<uint32_t> array_t;
    entry_t entry_; //!< entry vector
    char* buf_; //!< buffer to store labels for fast access
    boost::mutex rw_mtx_;

    /**
     @brief add docid-address pair into hash map
     @param unit docid-address pair
     */
    void map_add_(ID_ADRESS_PAIR unit)
    {
        uint32_t i = unit % ENTRY_SIZE;
        if (entry_.at(i) == NULL)
            entry_[i] = new slot_t();

        slot_t* p = entry_.at(i);
        i = p->find(unit);

        if (i != slot_t::NOT_FOUND)
        {
            (*p)[i] = unit;
            ++updated_num_;
            if (updated_num_ >= updated_bound_)
                compact_();
            return;
        }

        p->push_back(unit);
    }

    /**
     @brief delete from hash map
     @param docid it indicates the one need to be removed.
     */
    void map_del_(const uint32_t& docid)
    {
        ID_ADRESS_PAIR unit(docid);
        uint32_t i = unit % 1000000;
        if (entry_.at(i) == NULL)
            return;

        izenelib::am::DynArray<ID_ADRESS_PAIR>* p = entry_.at(i);
        i = p->find(unit);
        if (i != slot_t::NOT_FOUND)
            p->erase(i);
    }

    /**
     @brief get address of a given document.
     @param docid it indicates the document.
     @return 64 bit address.
     */
    uint64_t get_addr_(uint32_t docid)
    {
        ID_ADRESS_PAIR unit(docid);
        uint32_t i = unit % 1000000;
        if (entry_.at(i) == NULL)
            return -1;

        slot_t* p = entry_.at(i);
        i = p->find(unit);
        if (i != slot_t::NOT_FOUND)
        {
            return p->at(i).ADDR();
        }

        return -1;
    }

    /**
     @brief load map from file.
     */
    void load_map_()
    {
        fseek(map_f_, 0, SEEK_SET);
        if (fread(&doc_num_, sizeof(uint32_t), 1, map_f_) != 1)
            return;
        if (fread(&updated_num_, sizeof(uint32_t), 1, map_f_) != 1)
            return;

        uint32_t i = 0;
        IASSERT(fread(&i, sizeof(uint32_t), 1, map_f_) == 1);
        while (i != (uint32_t) -1)
        {
            entry_[i] = new slot_t();
            entry_[i]->load(map_f_);
            IASSERT(fread(&i, sizeof(uint32_t), 1, map_f_) == 1);
        }
    }

    /**
     @brief save hash map onto file.
     */
    void save_map_(bool del = false)
    {
      if (map_f_ == NULL)
        return;

        fseek(map_f_, 0, SEEK_SET);
        IASSERT(fwrite(&doc_num_, sizeof(uint32_t), 1, map_f_) == 1);
        IASSERT(fwrite(&updated_num_, sizeof(uint32_t), 1, map_f_) == 1);

        for (uint32_t i = 0; i < ENTRY_SIZE; i++)
        {
            if (entry_.at(i) == NULL)
                continue;
            IASSERT(fwrite(&i, sizeof(uint32_t), 1, map_f_) == 1);
            entry_.at(i)->save(map_f_);
            if (del)
                delete entry_.at(i);
        }

        uint32_t i = -1;
        IASSERT(fwrite(&i, sizeof(uint32_t), 1, map_f_) == 1);

        fflush(map_f_);

      if (del)
      {
        fclose(map_f_);
        map_f_ = NULL;
      }

    }

    /**
     @brief compact the entire data after a certain times updates.
     */
    void compact_()
    {
        updated_num_ = 0;
        fflush(f_);
        save_map_();
        //load_map_();

        if (buf_ != NULL)
            free(buf_);
        buf_ = NULL;

        addr_id_pairs_t addrs;
        for (uint32_t i = 0; i < ENTRY_SIZE; i++)
        {
            if (entry_.at(i) == NULL)
                continue;

            slot_t* p = entry_.at(i);
            for (uint32_t j = 0; j < p->length(); ++j)
                addrs.push_back(ADRESS_ID_PAIR(p->at(j).ADDR(),
                        p->at(j).DOCID()));
        }
        addrs.sort();

        for (uint32_t i = 0; i < ENTRY_SIZE; i++)
        {
            if (entry_.at(i) == NULL)
                continue;

            delete entry_.at(i);
            entry_[i] = NULL;
        }

        FILE* f = fopen((nm_ + ".tmp").c_str(), "w+");
        uint32_t BUF_SIZE = 1000000;
        char* content = (char*)malloc(1000000);
        uint32_t len;
        for (uint32_t i = 0; i < addrs.length(); ++i)
        {
          fseek(f_, addrs.at(i).ADDR(), SEEK_SET);
          IASSERT(fread(&len, sizeof(uint16_t), 1, f_)==1);
          if (len>BUF_SIZE)
          {
            content = (char*)realloc(content, len);
            BUF_SIZE = len;
          }

          IASSERT(fread(content, len, 1, f_)==1);
          map_add_(ID_ADRESS_PAIR(addrs.at(i).DOCID(), ftell(f)));


          IASSERT(fwrite(&len, sizeof(uint16_t), 1, f)==1);
          IASSERT(fwrite(content, len, 1, f)==1);
        }
        fclose(f_);
        fclose(f);

        free(content);
        boost::filesystem::remove(nm_.c_str());
        boost::filesystem::rename((nm_ + ".tmp").c_str(), nm_.c_str());
        f_ = fopen(nm_.c_str(), "r+");

        fflush(f_);
        save_map_();
    }

    /**
     @brief initialize buffer for labels.
     */
    inline void init_buf_()
    {
        if (buf_ != NULL)
            return;

        fseek(f_, 0, SEEK_END);

        uint64_t len = ftell(f_);
        fseek(f_, 0, SEEK_SET);
        if (len > CACHE_SIZE)
        {
            buf_ = (char*) malloc(CACHE_SIZE);
            IASSERT(fread(buf_, CACHE_SIZE, 1, f_) == 1);
        }
        else if(len>0)
        {
            buf_ = (char*) malloc(len);
            IASSERT(fread(buf_, len, 1, f_) == 1);
        }
    }

public:
    /**
     @brief a constructor.
     @param filenm file name
     */
    DocTable(const std::string& filenm) :
        nm_(filenm)
    {
        updated_num_ = 0;
        entry_.reserve(ENTRY_SIZE);
        for (uint32_t i = 0; i < ENTRY_SIZE; i++)
            entry_.push_back(NULL);

        f_ = fopen(filenm.c_str(), "r+");
        if (f_ == NULL)
        {
            f_ = fopen(filenm.c_str(), "w+");
        }

        std::string fn = filenm;
        fn += ".map";
        map_f_ = fopen(fn.c_str(), "r+");
        if (map_f_ == NULL)
        {
            map_f_ = fopen(fn.c_str(), "w+");
        }
        else
            load_map_();

        updated_bound_ = 100000;
        buf_ = NULL;
    }

    /**
     @brief a destructor
     */
    ~DocTable()
    {
      boost::mutex::scoped_lock lock(rw_mtx_);
      if (f_ != NULL)
        fclose(f_);

      if (buf_ != NULL)
        free(buf_);
      save_map_(true);
    }

  void release()
  {
    boost::mutex::scoped_lock lock(rw_mtx_);
    fclose(f_);
    f_ = NULL;

    if (buf_ != NULL)
      free(buf_);
    buf_ = NULL;
    save_map_(true);
  }

    /**
     @brief flush all data on to memory without deallocation.
     */
    void flush()
    {
        boost::mutex::scoped_lock lock(rw_mtx_);
        fflush(f_);
        save_map_();
        //load_map_();

        if (buf_ != NULL)
            free(buf_);
        buf_ = NULL;
    }

    /**
     @brief set bound of times of updates which control the occurence of compacting.
     */
    void set_update_bound(uint32_t t)
    {
        updated_bound_ = t;
    }

    /**
     @brief insert labels of a document. it would be updating operation if the doc is already there.
     @param docid document ID.
     @param labels label collcection of the document.
     @return return true if insertion is successful
     */
  bool insert_doc(docid_t docid, const char* content, uint16_t len)
    {
      boost::mutex::scoped_lock lock(rw_mtx_);
      if (len == 0)
        return true;

        fseek(f_, 0, SEEK_END);
        uint64_t a = ftell(f_);

        IASSERT(fwrite(&len, sizeof(uint16_t),1,f_)==1);
        IASSERT(fwrite(content, len, 1, f_)==1);

        map_add_(ID_ADRESS_PAIR(docid, a));
        return true;
    }

    /**
     @brief delete labels of a certain document
     @param docid document ID.
     */
    void delete_doc(docid_t docid)
    {
      boost::mutex::scoped_lock lock(rw_mtx_);
        map_del_(docid);
        ++updated_num_;

        if (updated_num_ >= updated_bound_)
            compact_();
    }

    /**
     @brief get labels of one document.
     @param docid document ID.
     @param labels as a return value, label collcection of the document.
     @return return true if insertion is successful
     */
    uint16_t get_doc(docid_t docid, char** content)
    {
      boost::mutex::scoped_lock lock(rw_mtx_);
        init_buf_();

        *content = NULL;
        uint64_t addr = get_addr_(docid);
        if (addr == (uint64_t) -1)
        {
          return 0;
        }


        array_t lbls;
        if (addr + sizeof(uint16_t) < CACHE_SIZE && *(uint16_t*) (buf_ + addr)
            + addr + sizeof(uint16_t) <= CACHE_SIZE)
        {
          *content = (char*)malloc(*(uint16_t*) (buf_ + addr));
          memcpy(*content, buf_+addr+sizeof(uint16_t), *(uint16_t*)(buf_ + addr));
          return *(uint16_t*) (buf_ + addr);
        }
        else
        {
          fseek(f_, addr, SEEK_SET);
          uint16_t len;
          IASSERT(fread(&len, sizeof(uint16_t), 1, f_)==1);
          *content = (char*)malloc(len);
          IASSERT(fread(*content, len, 1, f_)==1);
          return len;
        }
    }

    /**
     @brief get labels of some document.
     @param docIdList document ID colloction.
     @param labels as a return value, label collcections of corresponding document.
     @return return true if insertion is successful
     */
    bool get_doc(const std::vector<docid_t>& docIdList,
                 std::vector<char*>& contents, std::vector<uint16_t>& lens)
    {
      boost::mutex::scoped_lock lock(rw_mtx_);
        contents.clear();
        lens.clear();
        init_buf_();

//      struct timeval tvafter, tvpre;
//      struct timezone tz;

//      gettimeofday(&tvpre, &tz);
        addr_id_pairs_t array;
        for (uint32_t i = 0; i < docIdList.size(); ++i)
        {
            uint64_t addr = get_addr_(docIdList[i]);
            if (addr == (uint64_t) -1)
                continue;

            array.push_back(ADRESS_ID_PAIR(addr, docIdList[i], i));
        }
        array.sort();
//      gettimeofday(&tvafter, &tz);
//      cout << "\n[getLabelsByDocIdList 1]: " << ((tvafter.tv_sec
//          - tvpre.tv_sec) * 1000. + (tvafter.tv_usec - tvpre.tv_usec)
//          / 1000.) / 1000. << std::endl;
//
//      gettimeofday(&tvpre, &tz);
        std::vector<char*> ptrs;
        std::vector<uint16_t> ls;
        ptrs.reserve(array.length());
        ls.reserve(array.length());
        char* cont;
        uint16_t len;
        for (uint32_t k = 0; k < array.length(); ++k)
        {
            uint64_t addr = array.at(k).ADDR();
            if (addr + sizeof(uint16_t) < CACHE_SIZE && *(uint16_t*) (buf_ + addr)
                + addr + sizeof(uint16_t) <= CACHE_SIZE)
            {
              len = *(uint16_t*) (buf_ + addr);
              cont = (char*)malloc(*(uint16_t*) (buf_ + addr));
              memcpy(cont, buf_+addr+sizeof(uint16_t), *(uint16_t*) (buf_ + addr));
            }
            else
            {
              fseek(f_, addr, SEEK_SET);
              IASSERT(fread(&len, sizeof(uint16_t), 1, f_)==1);
              cont = (char*)malloc(len);
              IASSERT(fread(cont, len, 1, f_)==1);
            }

            ptrs.push_back(cont);
            ls.push_back(len);
        }
//      gettimeofday(&tvafter, &tz);
//      cout << "\n[getLabelsByDocIdList 2]: " << ((tvafter.tv_sec
//          - tvpre.tv_sec) * 1000. + (tvafter.tv_usec - tvpre.tv_usec)
//          / 1000.) / 1000. << std::endl;
//
//      gettimeofday(&tvpre, &tz);
        for (uint32_t k = 0; k < docIdList.size(); ++k)
        {
          contents.push_back(NULL);
          lens.push_back(0);
        }

        for (uint32_t k = 0; k < ptrs.size(); ++k)
        {
            contents[array.at(k).INDEX()] = ptrs[k];
            lens[array.at(k).INDEX()] = ls[k];
        }
//      gettimeofday(&tvafter, &tz);
//      cout << "\n[getLabelsByDocIdList 3]: " << ((tvafter.tv_sec
//          - tvpre.tv_sec) * 1000. + (tvafter.tv_usec - tvpre.tv_usec)
//          / 1000.) / 1000. << std::endl;

        return true;
    }

protected:
    std::string nm_;//!< file name
    FILE* f_; //!< label file integer handler.
    FILE* map_f_;//!< hash map file handler.
    uint32_t doc_num_;//!< number of document.
    uint32_t updated_num_;//!< update times since last compacting.
    uint32_t updated_bound_;//!< upper bound of update times before compacting.

};

}
#endif
