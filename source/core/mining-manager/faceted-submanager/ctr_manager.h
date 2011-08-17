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

#include <boost/thread/mutex.hpp>
#include <boost/thread/shared_mutex.hpp>

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

    /**
     * Get click-count for each document.
     * @param docIdList [IN]
     * @param posClickCountList [OUT]
     */
    void getClickCountListByDocIdList(
            const std::vector<unsigned int>& docIdList,
            std::vector<std::pair<size_t, count_t> >& posClickCountList);

    void getClickCountListByDocIdList(
            const std::vector<unsigned int>& docIdList,
            std::vector<count_t>& clickCountList);

    /**
     * Get click-count for specified document
     * @param docId
     * @return click count
     */
    count_t getClickCountByDocId(uint32_t docId);

    bool getClickCountByDocId(uint32_t docId, count_t& clickCount);

private:
    bool updateDB(uint32_t docId, count_t clickCount);

private:
    std::string filePath_;
    size_t docNum_;

    std::vector<count_t> docClickCountList_;

    DBType* db_;

    mutable boost::shared_mutex mutex_;
};

NS_FACETED_END

#endif /* SF1R_CTR_MANAGER_H_ */
