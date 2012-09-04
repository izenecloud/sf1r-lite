#ifndef SF1R_B5MMANAGER_BRANDDB_H_
#define SF1R_B5MMANAGER_BRANDDB_H_

#include <string>
#include <vector>
#include <product-manager/product_price.h>
#include "b5m_types.h"
#include "b5m_helper.h"
#include <types.h>
#include <am/tc/BTree.h>
#include <am/tc/Hash.h>
#include <am/leveldb/Table.h>
#include <ir/id_manager/IDManager.h>
#include <am/succinct/fujimap/fujimap.hpp>
#include <glog/logging.h>
#include <boost/unordered_map.hpp>

namespace sf1r {


    class BrandDb
    {
    public:
        typedef uint128_t IdType;
        typedef uint32_t BidType;
        typedef izenelib::util::UString StringType;

        //typedef izenelib::ir::idmanager::_IDManager<StringType, BidType,
               //izenelib::util::NullLock,
               //izenelib::ir::idmanager::EmptyWildcardQueryHandler<StringType, BidType>,
               //izenelib::ir::idmanager::UniqueIDGenerator<StringType, BidType>,
               //izenelib::ir::idmanager::HDBIDStorage<StringType, BidType> >  IdManager;
        typedef izenelib::ir::idmanager::_IDManager<StringType, StringType, BidType,
                   izenelib::util::NullLock,
                   izenelib::ir::idmanager::EmptyWildcardQueryHandler<StringType, BidType>,
                   izenelib::ir::idmanager::UniqueIDGenerator<StringType, BidType>,
                   izenelib::ir::idmanager::HDBIDStorage<StringType, BidType>,
                   izenelib::ir::idmanager::UniqueIDGenerator<izenelib::util::UString, uint64_t>,
                   izenelib::ir::idmanager::EmptyIDStorage<izenelib::util::UString, uint64_t> > IdManager;


        typedef izenelib::am::succinct::fujimap::Fujimap<IdType, BidType> DbType;
        BrandDb(const std::string& path)
        : path_(path), db_path_(path+"/db"), id_path_(path+"/id"), tmp_path_(path+"/tmp")
        , db_(NULL), id_manager_(NULL), is_open_(false)
        {
        }

        ~BrandDb()
        {
            if(db_!=NULL)
            {
                delete db_;
            }
            if(id_manager_!=NULL)
            {
                delete id_manager_;
            }
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
            boost::filesystem::create_directories(id_path_);
            id_manager_ = new IdManager(id_path_+"/id");
            is_open_ = true;
            return true;
        }

        void set(const IdType& pid, const StringType& brand)
        {
            BidType bid;
            id_manager_->getTermIdByTermString(brand, bid);
            db_->setInteger(pid, bid);
        }

        bool get(const IdType& pid, StringType& brand)
        {
            BidType bid = db_->getInteger(pid);
            if(bid==(BidType)izenelib::am::succinct::fujimap::NOTFOUND)
            {
                return false;
            }
            return id_manager_->getTermStringByTermId(bid, brand);
        }

        bool flush()
        {
            LOG(INFO)<<"try flush bdb.."<<std::endl;
            if(db_->build()==-1)
            {
                LOG(ERROR)<<"fujimap build error"<<std::endl;
                return false;
            }
            db_->save(db_path_.c_str());
            id_manager_->flush();
            return true;
        }

    private:

    private:

        std::string path_;
        std::string db_path_;
        std::string id_path_;
        std::string tmp_path_;
        DbType* db_;
        IdManager* id_manager_;
        bool is_open_;
    };

}

#endif

