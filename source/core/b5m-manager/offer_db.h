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



    //typedef izenelib::am::tc::BTree<std::string, std::string> OfferDb;
    //typedef izenelib::am::leveldb::Table<KeyType, ValueType> DbType;

    //class OfferDb : public izenelib::am::tc::Hash<std::string, std::string>
    class OfferDb : public izenelib::am::leveldb::Table<std::string, std::string>
    {
    public:
        //typedef izenelib::am::tc::Hash<std::string, std::string> BaseType;
        typedef izenelib::am::leveldb::Table<std::string, std::string> BaseType;
        typedef boost::unordered_map<std::string, std::pair<std::string, bool> > MemType;
        OfferDb(const std::string& path): BaseType(path)
        {
        }

        bool simple_open()
        {
            return BaseType::open();
        }

        bool open()
        {
            if(!BaseType::open()) return false;

            LOG(INFO)<<"odb load to memory.."<<std::endl;
            BaseType::cursor_type it = BaseType::begin();
            std::string key;
            std::string value;
            while(true)
            {
                if(!BaseType::fetch(it, key, value)) break;
                std::pair<std::string, bool> mem_value(value, false);
                mem_db_.insert(std::make_pair(key, mem_value));
                BaseType::iterNext(it);
            }
            LOG(INFO)<<"odb size : "<<mem_db_.size()<<std::endl;
            return true;

        }

        bool get(const std::string& key, std::string& value) const
        {
            MemType::const_iterator it = mem_db_.find(key);
            if(it==mem_db_.end()) return false;
            value = it->second.first;
            if(value.empty()) return false;
            return true;
        }

        bool update(const std::string& key, const std::string& value)
        {
            mem_db_[key] = std::make_pair(value, true);
            return true;
        }

        bool del(const std::string& key)
        {
            return update(key, "");
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
    };

}

#endif

