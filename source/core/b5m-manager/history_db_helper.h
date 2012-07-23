#ifndef SF1R_B5MMANAGER_HISTORYDB_HELPER_H_
#define SF1R_B5MMANAGER_HISTORYDB_HELPER_H_

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
#include "history_db.h"

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
    class B5MHistoryDBHelper
    {
    public:
        typedef uint128_t  KeyType;
        typedef uint128_t  ValueType;
        typedef izenelib::am::succinct::fujimap::Fujimap<KeyType, ValueType> DbType;
        B5MHistoryDBHelper(const std::string& path)
        {
            db_imp_ = new HistoryDB(path);
        }

        ~B5MHistoryDBHelper()
        {
            delete db_imp_;
        }

        bool is_open() const
        {
            return db_imp_->is_open();
        }

        bool open()
        {
            return db_imp_->open();
        }

        bool offer_load_text(const std::string& path, bool text)
        {
            return db_imp_->olduuid_load_text(path, text);
        }
        bool pd_load_text(const std::string& path, bool text)
        {
            return db_imp_->olddocid_load_text(path, text);
        }

        bool offer_insert(const KeyType& key, const std::string& value)
        {
            return db_imp_->insert_olduuid(key, value);
        }
        bool pd_insert(const KeyType& key, const std::string& value)
        {
            return db_imp_->insert_olddocid(key, value);
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
            return db_imp_->get_olduuid(key, value);            
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
            return db_imp_->get_olddocid(key, value);            
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
            db_imp_->remove_olddocid(key, value);
        }

        bool flush()
        {
            return db_imp_->flush();
        }

        bool offer_flush()
        {
            return db_imp_->olduuid_flush();
        }
        bool pd_flush()
        {
            return db_imp_->olddocid_flush();
        }

    private:
        std::string path_;
        sf1r::HistoryDB* db_imp_;
    };
}

#endif

