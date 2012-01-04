#ifndef LOG_SERVER_STORAGE_H_
#define LOG_SERVER_STORAGE_H_

#include "LogServerCfg.h"
#include "DrumDispatcher.h"

#include <util/singleton.h>
#include <am/leveldb/Table.h>
#include <am/tokyo_cabinet/tc_fixdb.h>
#include <3rdparty/am/drum/drum.hpp>

#include <boost/shared_ptr.hpp>

namespace sf1r
{

class LogServerStorage
{
public:
    // DRUM <uuid, docids>
    typedef uint128_t drum_key_t;
    typedef std::vector<uint32_t> drum_value_t;
    typedef std::string drum_aux_t;

    typedef DrumDispatcher<
                drum_key_t,
                drum_value_t,
                drum_aux_t
    > DrumDispatcherImpl;

    typedef izenelib::drum::Drum<
        drum_key_t,
        drum_value_t,
        drum_aux_t,
        izenelib::am::leveldb::TwoPartComparator,
        izenelib::am::leveldb::Table,
        DrumDispatcher
    > DrumType;
    typedef boost::shared_ptr<DrumType> DrumPtr;

    // DB <docid, uuid>
    typedef izenelib::am::tc_fixdb<drum_key_t> KVDBType;
    typedef boost::shared_ptr<KVDBType> KVDBPtr;

public:
    static LogServerStorage* get()
    {
        return izenelib::util::Singleton<LogServerStorage>::get();
    }

    bool init()
    {
        // Initialize drum
        drum_.reset(
                new DrumType(
                    LogServerCfg::get()->getDrumName(),
                    LogServerCfg::get()->getDrumNumBuckets(),
                    LogServerCfg::get()->getDrumBucketBuffElemSize(),
                    LogServerCfg::get()->getDrumBucketByteSize(),
                    drumDispathcerImpl_
                ));

        if (!drum_)
        {
            std::cerr << "Failed to initialzie drum: " << LogServerCfg::get()->getDrumName() << std::endl;
            return false;
        }

        // Initialize <docid, uuid> DB
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
        try
        {
            if (drum_)
            {
                drum_->Synchronize();
                drum_->Dispose();
            }

            if (docidDB_)
            {
                docidDB_->close();
            }
        }
        catch (const std::exception& e)
        {
            std::cout << "LogServerStorage close: " <<e.what()<< std::endl;
        }
    }

    DrumDispatcherImpl& getDrumDispatcher()
    {
        return drumDispathcerImpl_;
    }

    const DrumPtr& getDrum()
    {
        return drum_;
    }

    const KVDBPtr& getDocidDB()
    {
        return docidDB_;
    }

private:
    DrumDispatcherImpl drumDispathcerImpl_;
    DrumPtr drum_;

    KVDBPtr docidDB_;
};

}

#endif /* LOG_SERVER_STORAGE_H_ */
