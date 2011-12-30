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
        uint128_t,
        std::vector<uint128_t>,
        std::string,
        izenelib::am::leveldb::TwoPartComparator,
        izenelib::am::leveldb::Table,
        DocidDispatcher
    > DrumType;
    typedef boost::shared_ptr<DrumType> DrumPtr;

    typedef izenelib::am::tc_hash<uint32_t, std::string> KVDBType;
    //typedef izenelib::am::sdb_fixedhash<uint32_t, count_t, izenelib::util::ReadWriteLock> KVDBType;
    typedef boost::shared_ptr<KVDBType> KVDBPtr;

public:
    static LogServerStorage* get()
    {
        return izenelib::util::Singleton<LogServerStorage>::get();
    }

    bool init()
    {
        drum_.reset(
                new DrumType(
                    LogServerCfg::get()->getDrumName(),
                    LogServerCfg::get()->getDrumNumBuckets(),
                    LogServerCfg::get()->getDrumBucketBuffElemSize(),
                    LogServerCfg::get()->getDrumBucketByteSize()
                ));

        // TODO: initialize KVDB

        return drum_;
    }

    DrumPtr getDrum() const
    {
        return drum_;
    }

    KVDBPtr getKVDB() const
    {
        return kvDB_;
    }

private:
    DrumPtr drum_;

    KVDBPtr kvDB_;
};

}

#endif /* LOG_SERVER_STORAGE_H_ */
