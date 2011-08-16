/**
 * @file ctr-manager.h
 * @author Zhongxia Li
 * @date Aug 16, 2011
 * @brief Click Through Rate Manager
 */
#ifndef SF1R_CTR_MANAGER_H_
#define SF1R_CTR_MANAGER_H_

#include "faceted_types.h"

#include <am/tokyo_cabinet/tc_hash.h>
#include <sdb/SequentialDB.h>

NS_FACETED_BEGIN

class CTRManager
{
public:
    typedef uint32_t count_t;
    //typedef izenelib::am::tc_hash<uint32_t, count_t> DBType;
    typedef izenelib::am::sdb_fixedhash<uint32_t, count_t, izenelib::util::ReadWriteLock> DBType;

public:
    CTRManager(const std::string& dirPath, size_t docNum = 0);

    ~CTRManager();

public:

    bool open();

    /**
     * Update ctr by internal document id.
     * @param docId, indicates which document to be updated.
     * @return true if success, or false
     */
    bool update(uint32_t docId);

private:
    bool updateDB(uint32_t docId, count_t clickCount);

private:
    std::string filePath_;
    size_t docNum_;

    std::vector<count_t> docClickCountList_;

    DBType* db_;
};

NS_FACETED_END

#endif /* SF1R_CTR_MANAGER_H_ */
