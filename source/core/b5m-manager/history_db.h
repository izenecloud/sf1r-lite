#ifndef SF1R_B5MMANAGER_HISTORYDB_H_
#define SF1R_B5MMANAGER_HISTORYDB_H_

#include <string>
#include <vector>
#include "b5m_helper.h"
#include <types.h>
#include <am/tc/BTree.h>
#include <am/tc/Hash.h>
#include <am/leveldb/Table.h>
#include <am/succinct/fujimap/fujimap.hpp>
#include <glog/logging.h>
#include <boost/unordered_map.hpp>

namespace sf1r {

    /* -----------------------------------*/
    /**
     * @Brief  store all the history product ids for the offer item.
     *         when the product id of the offer has changed, the old product id
     *         will be added here.
     *
     *         store all the offer ids for the product item.
     *         when a new offer coming, it will be added to this product item.
     *         when a offer update its product, old product will keep the offer,
     *         and new product will also add the moved offer.
     *         when offer was really deleted, all the products who hold the offer 
     *         should be updated by removing this offer from the product.
     */
    /* -----------------------------------*/
    class HistoryDB
    {
    public:
        typedef uint128_t  KeyType;
        typedef uint128_t  ValueType;
        typedef izenelib::am::succinct::fujimap::Fujimap<KeyType, ValueType> DbType;
        HistoryDB(const std::string& path)
        : path_(path)
          , is_open_(false)
          , offer_db_path_(path+"/offerdb")
          , offer_text_path_(path+"/offertext")
          , offer_tmp_path_(path+"/offertmp")
          , offer_db_(NULL)
          , offer_has_modify_(false)
          , pd_db_path_(path+"/pddb")
          , pd_text_path_(path+"/pdtext")
          , pd_tmp_path_(path+"/pdtmp")
          , pd_db_(NULL)
          , pd_has_modify_(false)
        {
        }

        ~HistoryDB()
        {
            if(offer_db_!=NULL)
            {
                delete offer_db_;
            }
            offer_text_.close();
            if(pd_db_!=NULL)
            {
                delete pd_db_;
            }
            pd_text_.close();
        }

        bool is_open() const
        {
            return is_open_;
        }

        bool open()
        {
            if(is_open_) return true;
            boost::filesystem::create_directories(path_);
            if(!open_internal(offer_tmp_path_, offer_db_, offer_db_path_, offer_text_path_,
                offer_text_, offer_has_modify_))
            {
                return false;
            }
            if(!open_internal(pd_tmp_path_, pd_db_, pd_db_path_, pd_text_path_,
                pd_text_, pd_has_modify_))
            {
                return false;
            }
            is_open_ = true;
            return true;
        }

        bool offer_load_text(const std::string& path, bool text = true)
        {
            return load_text(offer_db_, offer_text_, offer_has_modify_, path, text);
        }
        bool pd_load_text(const std::string& path, bool text = true)
        {
            return load_text(pd_db_, pd_text_, pd_has_modify_, path, text);
        }

        bool offer_insert(const KeyType& key, const std::string& value)
        {
            return offer_insert_(key, value, true);
        }
        bool pd_insert(const KeyType& key, const std::string& value)
        {
            return pd_insert_(key, value, true);
        }

        bool offer_insert(const std::string& soid, const std::string& spid)
        {
            return offer_insert(B5MHelper::StringToUint128(soid), spid);
        }
        bool pd_insert(const std::string& spid, const std::string& soid)
        {
            return pd_insert(B5MHelper::StringToUint128(spid), soid);
        }

        bool offer_get(const KeyType& key, std::string& value) const
        {
            size_t len = 0;
            const char* v = offer_db_->getString(key, len);
            if(v == NULL)
            {
                return false;
            }
            value = std::string(v, len);
            
            return true;
        }

        bool offer_get(const std::string& soid, std::string& spids) const
        {
            if(!offer_get(B5MHelper::StringToUint128(soid), spids)) return false;
            return true;
        }
        bool offer_get(const std::string& soid, std::set<std::string>& spid_vec) const
        {
            std::string spids;
            if(offer_get(soid, spids))
            {
                boost::algorithm::split(spid_vec, spids, boost::is_any_of(","));
                return true;
            }
            return false;
        }

        bool pd_get(const KeyType& key, std::string& value) const
        {
            size_t len = 0;
            const char* v = pd_db_->getString(key, len);
            if(v == NULL)
            {
                return false;
            }
            value = std::string(v, len);
            return true;
        }

        bool pd_get(const std::string& spid, std::string& soids) const
        {
            if(!pd_get(B5MHelper::StringToUint128(spid), soids)) return false;
            return true;
        }

        void pd_remove_offerid(const std::string& key, const std::string& value)
        {
            pd_remove_offerid(B5MHelper::StringToUint128(key), value);
        }
        // remove a single offer id which was deleted.
        void pd_remove_offerid(const KeyType& key, const std::string& value)
        {
            std::string soids;
            if(pd_get(key, soids))
            {
                LOG(INFO) << "removing a offerid from a product since the offerid was deleted." << endl;
                LOG(INFO) << "product : " << B5MHelper::Uint128ToString(key) <<
                    " old offerids: " << soids << endl;
                if(soids == value)
                {
                    // last offerid
                    soids = "";
                }
                else
                {
                    soids.replace(soids.find(value + ","), value.length() + 1, "");
                    soids.replace(soids.find("," + value), value.length() + 1, "");
                }
                LOG(INFO) << " after deleting : " << value << " new:" << soids << endl;
                pd_db_->setString(key, soids.c_str(), soids.size(), true);
                pd_has_modify_ = true;
                pd_text_ << B5MHelper::Uint128ToString(key) << ","<< soids << std::endl;
            }
        }

        bool flush()
        {
            bool ret = true;
            ret = offer_flush();
            ret = pd_flush() && ret;
            return ret;
        }

        bool offer_flush()
        {
            LOG(INFO)<<"try flush history offer db.."<<std::endl;
            return flush(offer_db_, offer_text_, offer_has_modify_, offer_db_path_);
        }
        bool pd_flush()
        {
            LOG(INFO)<<"try flush history pd db.."<<std::endl;
            return flush(pd_db_, pd_text_, pd_has_modify_, pd_db_path_);
        }

    private:
        bool open_internal(const std::string& tmppath, DbType*& opendb,
            const std::string& opendbpath, const std::string& opentextpath,
            std::ofstream& opentext, bool& hasmodify)
        {
            if(boost::filesystem::exists(tmppath))
            {
                boost::filesystem::remove_all(tmppath);
            }
            opendb = new DbType(tmppath.c_str());
            opendb->initFP(32);
            opendb->initTmpN(30000000);
            if(boost::filesystem::exists(opendbpath))
            {
                opendb->load(opendbpath.c_str());
            }
            else if(boost::filesystem::exists(opentextpath))
            {
                LOG(INFO)<<"db empty, loading text"<<std::endl;
                load_text(opendb, opentext, hasmodify, opentextpath, false);
                if(!flush(opendb, opentext, hasmodify, opendbpath)) return false;
            }
            opentext.open(opentextpath.c_str(), std::ios::out | std::ios::app);
            return true;
        }
        bool load_text(DbType* loaddb, std::ofstream& loadtext, bool& hasmodify,
            const std::string& path, bool text = true)
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
                KeyType ioid = B5MHelper::StringToUint128(soid);

                for(size_t i = 1; i < vec.size(); ++i)
                {
                    insert_(loaddb, hasmodify, loadtext, ioid, vec[i], text);
                }
            }
            ifs.close();
            return true;
        }

        bool flush(DbType* flushdb, std::ofstream& flushtext, bool& flushmodify, 
            const std::string& flushdbpath)
        {
            if(flushtext.is_open())
            {
                flushtext.flush();
            }
            if(flushmodify)
            {
                LOG(INFO)<<"building fujimap.."<<std::endl;
                if(flushdb->build()==-1)
                {
                    LOG(ERROR)<<"fujimap build error"<<std::endl;
                    return false;
                }
                boost::filesystem::remove_all(flushdbpath);
                flushdb->save(flushdbpath.c_str());
                flushmodify = false;
            }
            else
            {
                LOG(INFO)<<"db not change"<<std::endl;
            }
            return true;
        }

        bool offer_insert_(const KeyType& key, const std::string& value, bool text)
        {
            return insert_(offer_db_, offer_has_modify_, offer_text_, key, value, text);
        }
        bool pd_insert_(const KeyType& key, const std::string& value, bool text)
        {
            return insert_(pd_db_, pd_has_modify_, pd_text_, key, value, text);
        }
        bool insert_(DbType* insertdb, bool& insert_modified, std::ofstream& inserttext,
            const KeyType& key, const std::string& value, bool text)
        {
            if(value.empty())
                return false;
            size_t len = 0;
            const char* v = insertdb->getString(key, len);
            std::string evalue = std::string(v, len);
            if(v == NULL)
            {
                evalue = value;
            }
            else if(evalue.find(value) != std::string::npos)
            {
                return false;
            }
            else
            {
                evalue += "," + value;
            }
            insertdb->setString(key, evalue.c_str(), evalue.size(), true);
            insert_modified = true;
            if(text)
            {
                inserttext << B5MHelper::Uint128ToString(key) << ","<< evalue << std::endl;
            }
            return true;
        }

    private:
        std::string path_;
        bool is_open_;

        std::string offer_db_path_;
        std::string offer_text_path_;
        std::string offer_tmp_path_;
        DbType* offer_db_;
        std::ofstream offer_text_;
        bool offer_has_modify_;

        std::string pd_db_path_;
        std::string pd_text_path_;
        std::string pd_tmp_path_;
        DbType* pd_db_;
        std::ofstream pd_text_;
        bool pd_has_modify_;
    };
}

#endif

