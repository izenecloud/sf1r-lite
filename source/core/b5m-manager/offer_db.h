#ifndef SF1R_B5MMANAGER_OFFERDB_H_
#define SF1R_B5MMANAGER_OFFERDB_H_

#include <string>
#include <vector>
#include <product-manager/product_price.h>
#include "b5m_types.h"
#include <am/tc/BTree.h>
#include <am/tc/Hash.h>
#include <am/leveldb/Table.h>
#include <glog/logging.h>
#include <boost/unordered_map.hpp>

namespace sf1r {


    typedef std::string OfferDbKeyType;
    
    struct OfferDbValueType {
        std::string pid;
        std::string source;
        template<class Archive> 
        void serialize(Archive& ar, const unsigned int version) 
        {
            ar & pid & source;
        }

        bool empty() const
        {
            return pid.empty();
        }
    };

    //typedef izenelib::am::tc::BTree<std::string, std::string> OfferDb;
    //typedef izenelib::am::leveldb::Table<KeyType, ValueType> DbType;

    //class OfferDb : public izenelib::am::tc::Hash<std::string, std::string>
    class OfferDb : public izenelib::am::leveldb::Table<OfferDbKeyType, OfferDbValueType>
    {
    public:
        typedef OfferDbKeyType KeyType;
        typedef OfferDbValueType ValueType;
        //typedef izenelib::am::tc::Hash<std::string, std::string> BaseType;
        typedef izenelib::am::leveldb::Table<KeyType, ValueType> BaseType;
        typedef boost::unordered_map<KeyType, std::pair<ValueType, bool> > MemType;
        OfferDb(const std::string& path): BaseType(path), simple_(false)
        {
        }

        bool simple_open()
        {
            simple_ = true;
            return BaseType::open();
        }

        bool open()
        {
            if(!BaseType::open()) return false;

            LOG(INFO)<<"odb load to memory.."<<std::endl;
            BaseType::cursor_type it = BaseType::begin();
            KeyType key;
            ValueType value;
            while(true)
            {
                if(!BaseType::fetch(it, key, value)) break;
                std::pair<ValueType, bool> mem_value(value, false);
                mem_db_.insert(std::make_pair(key, mem_value));
                BaseType::iterNext(it);
            }
            LOG(INFO)<<"odb size : "<<mem_db_.size()<<std::endl;
            return true;

        }

        bool get(const KeyType& key, ValueType& value) const
        {
            if(!simple_)
            {
                MemType::const_iterator it = mem_db_.find(key);
                if(it==mem_db_.end()) return false;
                value = it->second.first;
                if(value.empty()) return false;
                return true;
            }
            else
            {
                return BaseType::get(key, value);
            }
        }

        bool has_key(const KeyType& key) const
        {
            ValueType value;
            return get(key, value);
        }

        bool update(const KeyType& key, const ValueType& value)
        {
            mem_db_[key] = std::make_pair(value, true);
            return true;
        }

        bool del(const KeyType& key)
        {
            return update(key, ValueType());
        }

        bool flush()
        {
            LOG(INFO)<<"odb flush size : "<<mem_db_.size()<<std::endl;
            uint64_t n=0;
            for(MemType::const_iterator it = mem_db_.begin(); it!=mem_db_.end(); ++it)
            {
                if(it->second.second) //modified
                {
                    BaseType::update(it->first, it->second.first);
                }
                ++n;
                if(n%10000==0)
                {
                    LOG(INFO)<<"write "<<n<<" odb items"<<std::endl;
                }
            }
            return BaseType::flush();
        }

    private:

        MemType mem_db_;
        bool simple_;
    };

}

#endif

