#ifndef SF1R_B5MMANAGER_HISTORYDB_H_
#define SF1R_B5MMANAGER_HISTORYDB_H_

#include <string>
#include <vector>
#include <types.h>
#include <am/tc/BTree.h>
#include <am/tc/Hash.h>
#include <am/leveldb/Table.h>
#include <am/succinct/fujimap/fujimap.hpp>
#include <glog/logging.h>
#include <boost/unordered_map.hpp>
#include <boost/algorithm/string.hpp>

namespace sf1r {

    /* -----------------------------------*/
    /**
     * @Brief  store all the history uuids for the offer item.
     *         when the product id of the offer has changed, the old product id
     *         will be added here.
     *
     *         store all the old docids of the offers for the product item.
     *         when an offer changed its product id, old product will keep the offer's docid,
     *         when an offer was really deleted, all the products who hold its docid 
     *         should be updated by removing this olddocid from the product.
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
          , olduuid_db_path_(path+"/olduuiddb")
          , olduuid_text_path_(path+"/olduuidtext")
          , olduuid_tmp_path_(path+"/olduuidtmp")
          , olduuid_db_(NULL)
          , olduuid_has_modify_(false)
          , olddocid_db_path_(path+"/olddociddb")
          , olddocid_text_path_(path+"/olddocidtext")
          , olddocid_tmp_path_(path+"/olddocidtmp")
          , olddocid_db_(NULL)
          , olddocid_has_modify_(false)
        {
        }

        ~HistoryDB()
        {
            if(olduuid_db_!=NULL)
            {
                delete olduuid_db_;
            }
            olduuid_text_.close();
            if(olddocid_db_!=NULL)
            {
                delete olddocid_db_;
            }
            olddocid_text_.close();
        }

        bool is_open() const
        {
            return is_open_;
        }

        bool open()
        {
            if(is_open_) return true;
            boost::filesystem::create_directories(path_);
            if(!open_internal(olduuid_tmp_path_, olduuid_db_, olduuid_db_path_, olduuid_text_path_,
                olduuid_text_, olduuid_has_modify_))
            {
                return false;
            }
            if(!open_internal(olddocid_tmp_path_, olddocid_db_, olddocid_db_path_, olddocid_text_path_,
                olddocid_text_, olddocid_has_modify_))
            {
                return false;
            }
            is_open_ = true;
            return true;
        }

        bool olduuid_load_text(const std::string& path, bool text = true)
        {
            return load_text(olduuid_db_, olduuid_text_, olduuid_has_modify_, path, text);
        }
        bool olddocid_load_text(const std::string& path, bool text = true)
        {
            return load_text(olddocid_db_, olddocid_text_, olddocid_has_modify_, path, text);
        }

        bool insert_olduuid(const KeyType& key, const std::string& value)
        {
            return insert_olduuid_(key, value, true);
        }
        bool insert_olddocid(const KeyType& key, const std::string& value)
        {
            return insert_olddocid_(key, value, true);
        }
        bool get_olduuid(const KeyType& key, std::string& value) const
        {
            size_t len = 0;
            const char* v = olduuid_db_->getString(key, len);
            if(v == NULL)
            {
                return false;
            }
            value = std::string(v, len);
            
            return true;
        }

        bool get_olduuid(const KeyType& soid, std::set<std::string>& spid_vec) const
        {
            std::string spids;
            if(get_olduuid(soid, spids))
            {
                boost::algorithm::split(spid_vec, spids, boost::is_any_of(","));
                return true;
            }
            return false;
        }

        bool get_olddocid(const KeyType& key, std::string& value) const
        {
            size_t len = 0;
            const char* v = olddocid_db_->getString(key, len);
            if(v == NULL)
            {
                return false;
            }
            value = std::string(v, len);
            return true;
        }

        // remove a single olddocid of the offer which was deleted from the product.
        void remove_olddocid(const KeyType& key, const std::string& value)
        {
            std::string soids;
            if(get_olddocid(key, soids))
            {
                //LOG(INFO) << "removing a offerid from a product since the offerid was deleted." << endl;
                //LOG(INFO) << "product : " << Uint128ToString(key) <<
                //    " old offerids: " << soids << endl;
                if(soids == value)
                {
                    // last offerid
                    soids = "";
                }
                else
                {
                    size_t r_pos = soids.find(value + ",");
                    if(r_pos == std::string::npos)
                    {
                        r_pos = soids.find("," + value);
                    }
                    if(r_pos != std::string::npos)
                        soids.replace(r_pos, value.length() + 1, "");
                }
                olddocid_db_->setString(key, soids.c_str(), soids.size(), true);
                olddocid_has_modify_ = true;
                olddocid_text_ << Uint128ToString(key) << ","<< soids << std::endl;
            }
        }

        bool flush()
        {
            bool ret = true;
            ret = olduuid_flush();
            ret = olddocid_flush() && ret;
            return ret;
        }

        bool olduuid_flush()
        {
            LOG(INFO)<<"try flush history olduuid db.."<<std::endl;
            return flush(olduuid_db_, olduuid_text_, olduuid_has_modify_, olduuid_db_path_);
        }
        bool olddocid_flush()
        {
            LOG(INFO)<<"try flush history olddocid db.."<<std::endl;
            return flush(olddocid_db_, olddocid_text_, olddocid_has_modify_, olddocid_db_path_);
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
                KeyType ioid = StringToUint128(soid);

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

        bool insert_olduuid_(const KeyType& key, const std::string& value, bool text)
        {
            return insert_(olduuid_db_, olduuid_has_modify_, olduuid_text_, key, value, text);
        }
        bool insert_olddocid_(const KeyType& key, const std::string& value, bool text)
        {
            return insert_(olddocid_db_, olddocid_has_modify_, olddocid_text_, key, value, text);
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
                inserttext << Uint128ToString(key) << ","<< evalue << std::endl;
            }
            return true;
        }

        static std::string Uint128ToString(const uint128_t& val)
        {
            static char tmpstr[33];
            sprintf(tmpstr, "%016llx%016llx", (unsigned long long) (val >> 64), (unsigned long long) val);
            return std::string(reinterpret_cast<const char *>(tmpstr), 32);
        }
        static uint128_t StringToUint128(const std::string& str)
        {
            unsigned long long high = 0, low = 0;
            sscanf(str.c_str(), "%016llx%016llx", &high, &low);
            return (uint128_t) high << 64 | (uint128_t) low;
        }

    private:
        std::string path_;
        bool is_open_;

        std::string olduuid_db_path_;
        std::string olduuid_text_path_;
        std::string olduuid_tmp_path_;
        DbType* olduuid_db_;
        std::ofstream olduuid_text_;
        bool olduuid_has_modify_;

        std::string olddocid_db_path_;
        std::string olddocid_text_path_;
        std::string olddocid_tmp_path_;
        DbType* olddocid_db_;
        std::ofstream olddocid_text_;
        bool olddocid_has_modify_;
    };
}

#endif

