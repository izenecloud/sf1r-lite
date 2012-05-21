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
#include <am/succinct/fujimap/fujimap.hpp>
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
        //typedef boost::unordered_map<KeyType, ValueType, uint128_hash > DbType;
        typedef izenelib::am::succinct::fujimap::Fujimap<KeyType, ValueType> DbType;
        OfferDb(const std::string& path)
        : path_(path), db_path_(path+"/db"), text_path_(path+"/text"), tmp_path_(path+"/tmp")
        , db_(NULL), is_open_(false), has_modify_(false)
        {
        }

        ~OfferDb()
        {
            if(db_!=NULL)
            {
                delete db_;
            }
            text_.close();
        }

        bool is_open() const
        {
            return is_open_;
        }

        bool open()
        {
            if(is_open_) return true;
            boost::filesystem::create_directories(path_);
            if(boost::filesystem::exists(tmp_path_))
            {
                boost::filesystem::remove_all(tmp_path_);
            }
            db_ = new DbType(tmp_path_.c_str());
            db_->initFP(32);
            db_->initTmpN(30000000);
            if(boost::filesystem::exists(db_path_))
            {
                db_->load(db_path_.c_str());
            }
            else if(boost::filesystem::exists(text_path_))
            {
                LOG(INFO)<<"db empty, loading text"<<std::endl;
                load_text(text_path_, false);
            }
            text_.open(text_path_.c_str());
            is_open_ = true;
            return true;
        }

        bool load_text(const std::string& path, bool text = true)
        {
            LOG(INFO)<<"loading text "<<path<<std::endl;
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
                KeyType ioid = B5MHelper::StringToUint128(soid);
                ValueType ipid = B5MHelper::StringToUint128(spid);
                insert_(ioid, ipid, text);
            }
            ifs.close();
            if(!flush()) return false;
            return true;
        }

        bool insert(const KeyType& key, const ValueType& value)
        {
            return insert_(key, value, true);
        }

        bool insert(const std::string& soid, const std::string& spid)
        {
            return insert(B5MHelper::StringToUint128(soid), B5MHelper::StringToUint128(spid));
        }

        bool get(const KeyType& key, ValueType& value) const
        {
            value = db_->getInteger(key);
            if(value==(ValueType)izenelib::am::succinct::fujimap::NOTFOUND)
            {
                return false;
            }
            return true;
        }

        bool get(const std::string& soid, std::string& spid) const
        {
            uint128_t pid;
            if(!get(B5MHelper::StringToUint128(soid), pid)) return false;
            spid = B5MHelper::Uint128ToString(pid);
            return true;
        }

        //bool erase(const KeyType& key)
        //{
            //has_modify_ = true;
            //return db_.erase(key)>0;
        //}

        //bool erase(const std::string& soid)
        //{
            //return erase(B5MHelper::StringToUint128(soid));
        //}

        bool flush()
        {
            LOG(INFO)<<"try flush odb.."<<std::endl;
            text_.flush();
            LOG(INFO)<<"building fujimap.."<<std::endl;
            if(!db_->build())
            {
                LOG(ERROR)<<"fujimap build error"<<std::endl;
                return false;
            }
            return true;
        }

    private:
        bool insert_(const KeyType& key, const ValueType& value, bool text)
        {
            ValueType evalue = db_->getInteger(key);
            if(evalue==value)
            {
                return false;
            }
            db_->setInteger(key, value);
            if(text)
            {
                text_<<B5MHelper::Uint128ToString(key)<<","<<B5MHelper::Uint128ToString(value)<<std::endl;
            }
            return true;
        }

    private:

        std::string path_;
        std::string db_path_;
        std::string text_path_;
        std::string tmp_path_;
        DbType* db_;
        std::ofstream text_;
        bool is_open_;
        bool has_modify_;
    };

}

#endif

