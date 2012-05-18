#ifndef SF1R_B5MMANAGER_OFFERDB_H_
#define SF1R_B5MMANAGER_OFFERDB_H_

#include <string>
#include <vector>
#include <product-manager/product_price.h>
#include "b5m_types.h"
#include "b5m_helper.h"
#include <types.h>
#include <am/tc/BTree.h>
#include <am/tc/Hash.h>
#include <am/leveldb/Table.h>
#include <glog/logging.h>
#include <boost/unordered_map.hpp>

namespace sf1r {


    //typedef std::string OfferDbKeyType;
    
    //struct OfferDbValueType {
        //std::string pid;
        //std::string source;
        //template<class Archive> 
        //void serialize(Archive& ar, const unsigned int version) 
        //{
            //ar & pid & source;
        //}

        //bool empty() const
        //{
            //return pid.empty();
        //}
    //};

    //typedef izenelib::am::tc::BTree<std::string, std::string> OfferDb;
    //typedef izenelib::am::leveldb::Table<KeyType, ValueType> DbType;

    //class OfferDb : public izenelib::am::tc::Hash<std::string, std::string>
    class OfferDb
    {
    public:
        typedef uint128_t KeyType;
        typedef uint128_t ValueType;
        typedef boost::unordered_map<KeyType, ValueType, uint128_hash > DbType;
        typedef DbType::iterator iterator;
        typedef DbType::const_iterator const_iterator;
        OfferDb(const std::string& path, const std::string& tmp_path = "./fuji.tmp"): path_(path), is_open_(false), has_modify_(false)
        {
        }

        iterator begin()
        {
            return db_.begin();
        }

        const_iterator begin() const
        {
            return db_.begin();
        }

        iterator end()
        {
            return db_.end();
        }

        const_iterator end() const
        {
            return db_.end();
        }

        bool is_open() const
        {
            return is_open_;
        }

        bool open()
        {
            if(is_open_) return true;
            LOG(INFO)<<"Loading odb..."<<std::endl;
            load(path_);
            is_open_ = true;
            return true;
        }

        bool load(const std::string& path)
        {
            LOG(INFO)<<"loading file "<<path<<std::endl;
            std::ifstream ifs(path.c_str());
            std::string line;
            uint32_t i = 0;
            while( getline(ifs,line))
            {
                ++i;
                if(i%100000==0)
                {
                    LOG(INFO)<<"loading "<<i<<" item"<<std::endl;
                }
                boost::algorithm::trim(line);
                std::vector<std::string> vec;
                boost::algorithm::split(vec, line, boost::is_any_of(","));
                if(vec.size()<2) continue;
                const std::string& soid = vec[0];
                const std::string& spid = vec[1];
                insert(soid, spid);
            }
            ifs.close();
            return true;
        }

        bool insert(const KeyType& key, const ValueType& value)
        {
            bool modify = false;
            iterator it = db_.find(key);
            if(it==db_.end())
            {
                db_.insert(std::make_pair(key, value));
                modify = true;
            }
            else
            {
                if(value!=it->second)
                {
                    it->second = value;
                    modify = true;
                }
            }
            if(modify) has_modify_ = true;
            return modify;
        }

        bool insert(const std::string& soid, const std::string& spid)
        {
            return insert(B5MHelper::StringToUint128(soid), B5MHelper::StringToUint128(spid));
        }

        bool get(const KeyType& key, ValueType& value) const
        {
            DbType::const_iterator it = db_.find(key);
            if(it==db_.end()) return false;
            value = it->second;
            return true;
        }

        bool get(const std::string& soid, std::string& spid) const
        {
            uint128_t pid;
            if(!get(B5MHelper::StringToUint128(soid), pid)) return false;
            spid = B5MHelper::Uint128ToString(pid);
            return true;
        }

        bool erase(const KeyType& key)
        {
            has_modify_ = true;
            return db_.erase(key)>0;
        }

        bool erase(const std::string& soid)
        {
            return erase(B5MHelper::StringToUint128(soid));
        }

        bool flush()
        {
            LOG(INFO)<<"try flush odb.."<<std::endl;
            if(!has_modify_)
            {
                LOG(INFO)<<"not modified, return"<<std::endl;
                return true;
            }
            LOG(INFO)<<"Saving odb, size: "<<db_.size()<<std::endl;
            std::ofstream ofs(path_.c_str());
            for(DbType::const_iterator it = db_.begin(); it!=db_.end(); ++it)
            {
                ofs<<B5MHelper::Uint128ToString(it->first)<<","<<B5MHelper::Uint128ToString(it->second)<<std::endl;
            }
            ofs.close();
            has_modify_ = false;
            return true;
        }

        //bool simple_open()
        //{
            //simple_ = true;
            //return BaseType::open();
        //}

        //bool open()
        //{
            //if(!BaseType::open()) return false;

            //LOG(INFO)<<"odb load to memory.."<<std::endl;
            //BaseType::cursor_type it = BaseType::begin();
            //KeyType key;
            //ValueType value;
            //while(true)
            //{
                //if(!BaseType::fetch(it, key, value)) break;
                //std::pair<ValueType, bool> mem_value(value, false);
                //mem_db_.insert(std::make_pair(key, mem_value));
                //BaseType::iterNext(it);
            //}
            //LOG(INFO)<<"odb size : "<<mem_db_.size()<<std::endl;
            //return true;

        //}
        //bool get(const KeyType& key, ValueType& value) const
        //{
            //if(!simple_)
            //{
                //MemType::const_iterator it = mem_db_.find(key);
                //if(it==mem_db_.end()) return false;
                //value = it->second.first;
                //if(value.empty()) return false;
                //return true;
            //}
            //else
            //{
                //return BaseType::get(key, value);
            //}
        //}

        //bool has_key(const KeyType& key) const
        //{
            //ValueType value;
            //return get(key, value);
        //}

        //bool update(const KeyType& key, const ValueType& value)
        //{
            //mem_db_[key] = std::make_pair(value, true);
            //return true;
        //}

        //bool del(const KeyType& key)
        //{
            //return update(key, ValueType());
        //}

        //bool flush()
        //{
            //LOG(INFO)<<"odb flush size : "<<mem_db_.size()<<std::endl;
            //uint64_t n=0;
            //for(MemType::const_iterator it = mem_db_.begin(); it!=mem_db_.end(); ++it)
            //{
                //if(it->second.second) //modified
                //{
                    //BaseType::update(it->first, it->second.first);
                //}
                //++n;
                //if(n%10000==0)
                //{
                    //LOG(INFO)<<"write "<<n<<" odb items"<<std::endl;
                //}
            //}
            //return BaseType::flush();
        //}

    private:

        std::string path_;
        DbType db_;
        bool is_open_;
        bool has_modify_;
    };

}

#endif

