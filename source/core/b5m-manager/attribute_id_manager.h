#ifndef SF1R_B5MMANAGER_ATTRIBUTEIDMANAGER_H_
#define SF1R_B5MMANAGER_ATTRIBUTEIDMANAGER_H_
#include "b5m_types.h"
#include <boost/filesystem.hpp>
#include <am/leveldb/Table.h>
#include <am/sequence_file/ssfr.h>

namespace sf1r {

    class AttributeIdManager {


    public:

        typedef izenelib::util::UString UString;
        typedef izenelib::am::leveldb::Table<UString, uint32_t> NameDb;
        typedef izenelib::am::leveldb::Table<uint32_t, UString> IdDb;

        AttributeIdManager(const std::string& path): path_(path), max_id_(0)
        {
            if(!boost::filesystem::exists(path))
            {
                boost::filesystem::create_directories(path);
            }
            name_db_ = new NameDb(path+"/namedb");
            name_db_->open();
            id_db_ = new IdDb(path+"/iddb");
            id_db_->open();
            izenelib::am::ssf::Util<>::Load(path_+"/maxid", max_id_);
        }
        ~AttributeIdManager()
        {
            delete name_db_;
            delete id_db_;
        }

        void Set(const UString& text, uint32_t id)
        {
            name_db_->insert(text, id);
            id_db_->insert(id, text);
            if(id>max_id_) max_id_ = id;
        }

        bool getDocIdByDocName(const UString& text, uint32_t& id)
        {
            return name_db_->get(text, id);
        }

        bool getDocNameByDocId(uint32_t id, UString& text)
        {
            return id_db_->get(id, text);
        }

        uint32_t getMaxDocId() const
        {
            return max_id_;
        }

        void flush()
        {
            name_db_->flush();
            id_db_->flush();
            izenelib::am::ssf::Util<>::Save(path_+"/maxid", max_id_);
        }

        void close()
        {
            name_db_->close();
            id_db_->close();
        }


    private:

    private:
        std::string path_;
        NameDb* name_db_;
        IdDb* id_db_;
        uint32_t max_id_;
    };
}

#endif

