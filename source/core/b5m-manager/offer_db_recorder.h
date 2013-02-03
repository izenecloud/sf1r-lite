#ifndef SF1R_B5MMANAGER_OFFERDBRECORDER_H_
#define SF1R_B5MMANAGER_OFFERDBRECORDER_H_

#include <string>
#include <vector>
#include <product-manager/product_price.h>
#include "b5m_types.h"
#include "b5m_helper.h"
#include "offer_db.h"
#include <types.h>
#include <am/tc/BTree.h>
#include <am/tc/Hash.h>
#include <am/leveldb/Table.h>
#include <am/succinct/fujimap/fujimap.hpp>
#include <glog/logging.h>
#include <boost/unordered_map.hpp>

namespace sf1r {


    class OfferDbRecorder
    {
    public:
        typedef uint128_t KeyType;
        typedef uint128_t ValueType;
        typedef uint32_t FlagType;
        //typedef boost::unordered_map<KeyType, ValueType, uint128_hash > DbType;
        typedef izenelib::am::succinct::fujimap::Fujimap<KeyType, ValueType> DbType;
        typedef izenelib::am::succinct::fujimap::Fujimap<KeyType, FlagType> FlagDbType;
        OfferDbRecorder(OfferDb* odb, OfferDb* last_odb = NULL)
        : odb_(odb), last_odb_(last_odb)
        {
        }

        ~OfferDbRecorder()
        {
        }

        bool open()
        {
            if(!odb_->open()) return false;
            if(last_odb_!=NULL)
            {
                if(!last_odb_->open()) return false;
            }
            return true;
        }

        bool is_open() const
        {
            return odb_->is_open();
        }

        bool get(const KeyType& key, ValueType& value, bool& changed) const
        {
            if(!odb_->get(key, value)) return false;
            changed = true;
            if(last_odb_!=NULL)
            {
                changed = false;
                ValueType last_value;
                if(!last_odb_->get(key, last_value))
                {
                    changed = true;
                }
                else
                {
                    if(last_value!=value) 
                    {
                        changed = true;
                    }
                }
            }
            return true;
        }

        bool get(const std::string& soid, std::string& spid, bool& changed) const
        {
            uint128_t pid;
            if(!get(B5MHelper::StringToUint128(soid), pid, changed)) return false;
            spid = B5MHelper::Uint128ToString(pid);
            return true;
        }
        bool get_last(const KeyType& key, ValueType& value) const
        {
            if(last_odb_!=NULL)
            {
                return last_odb_->get(key, value);
            }
            return false;
        }

        bool get_last(const std::string& soid, std::string& spid) const
        {
            uint128_t pid;
            if(!get_last(B5MHelper::StringToUint128(soid), pid)) return false;
            spid = B5MHelper::Uint128ToString(pid);
            return true;
        }

    private:

        OfferDb* odb_;
        OfferDb* last_odb_;
    };

}

#endif

