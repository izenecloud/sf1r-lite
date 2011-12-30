#ifndef LOG_SERVER_STORAGE_H_
#define LOG_SERVER_STORAGE_H_

#include "LogServerCfg.h"
#include "DocidDispatcher.h"

#include <util/singleton.h>
#include <am/leveldb/Table.h>
#include <am/tokyo_cabinet/tc_hash.h>
#include <3rdparty/am/drum/drum.hpp>

#include <boost/shared_ptr.hpp>

namespace sf1r
{

class LogServerStorage
{
public:
    typedef izenelib::drum::Drum<
        std::string,
        std::vector<uint32_t>,
        std::string,
        izenelib::am::leveldb::TwoPartComparator,
        izenelib::am::leveldb::Table,
        DocidDispatcher
    > DrumType;
    typedef boost::shared_ptr<DrumType> DrumPtr;

    typedef izenelib::am::tc_hash<uint32_t, std::string> KVDBType;
    typedef boost::shared_ptr<KVDBType> KVDBPtr;

public:
    static LogServerStorage* get()
    {
        return izenelib::util::Singleton<LogServerStorage>::get();
    }

    bool init()
    {
        // initialize drum
        drum_.reset(
                new DrumType(
                    LogServerCfg::get()->getDrumName(),
                    LogServerCfg::get()->getDrumNumBuckets(),
                    LogServerCfg::get()->getDrumBucketBuffElemSize(),
                    LogServerCfg::get()->getDrumBucketByteSize()
                ));

        if (!drum_)
        {
            std::cerr << "Failed to initialzie drum: " << LogServerCfg::get()->getDrumName() << std::endl;
            return false;
        }

        // initialize <docid, uuid> DB
        docidDB_.reset(new KVDBType(LogServerCfg::get()->getDocidDBName()));
        if (!docidDB_ || !docidDB_->open())
        {
            std::cerr << "Failed to initialzie docid DB: " << LogServerCfg::get()->getDocidDBName() << std::endl;
            return false;
        }

        return true;
    }

    void close()
    {
        if (drum_)
        {
            //drum_->Synchronize();
            drum_->Dispose();
        }

        if (docidDB_)
        {
            docidDB_->close();
        }
    }

    DrumPtr getDrum() const
    {
        return drum_;
    }

    KVDBPtr getDocidDB() const
    {
        return docidDB_;
    }

private:
    DrumPtr drum_;
    KVDBPtr docidDB_;
};

}

#endif /* LOG_SERVER_STORAGE_H_ */
