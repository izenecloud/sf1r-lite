/**
 * @file ctr-manager.h
 * @author Zhongxia Li
 * @date Aug 16, 2011
 * @brief Click Through Rate Manager
 */
#ifndef SF1R_CTR_MANAGER_H_
#define SF1R_CTR_MANAGER_H_

#include "faceted_types.h"

#include <common/NumericPropertyTable.h>

#include <am/tokyo_cabinet/tc_hash.h>
#include <sdb/SequentialDB.h>

#include <boost/thread/shared_mutex.hpp>

NS_FACETED_BEGIN

class CTRManager
{
public:
    static const std::string kCtrPropName;

    typedef izenelib::am::sdb_fixedhash<docid_t, count_t, izenelib::util::ReadWriteLock> DBType;

    typedef boost::shared_mutex MutexType;
    typedef boost::shared_lock<MutexType> ScopedReadLock;
    typedef boost::unique_lock<MutexType> ScopedWriteLock;

    CTRManager(const std::string& dirPath, size_t docNum = 0);

    ~CTRManager();

    MutexType& getMutex() const { return mutex_; }

    bool open();

    // resize
    void updateDocNum(size_t docNum);

    /**
     * Update ctr by internal document id.
     * @param docId, indicates which document to be updated.
     * @return true if success, or false
     */
    bool update(docid_t docId);

    /**
     * Get click-count for each document.
     * @param docIdList [IN]
     * @param posClickCountList [OUT]
     * @return true for @p posClickCountList contains positive value,
     *         false for @p posClickCountList is empty or its values are all zero.
     */
    bool getClickCountListByDocIdList(
            const std::vector<docid_t>& docIdList,
            std::vector<std::pair<size_t, count_t> >& posClickCountList);

    bool getClickCountListByDocIdList(
            const std::vector<unsigned int>& docIdList,
            std::vector<count_t>& clickCountList);

    boost::shared_ptr<NumericPropertyTableBase> getPropertyTable() const
    {
        return docClickCountList_;
    }

    /**
     * Get click-count for specified document
     * @param docId
     * @return click count
     */
    count_t getClickCountByDocId(docid_t docId);

private:
    bool updateDB(docid_t docId, count_t clickCount);

private:
    std::string filePath_;
    size_t docNum_;

    boost::shared_ptr<NumericPropertyTable<count_t> > docClickCountList_;

    DBType* db_;

    mutable MutexType mutex_;
};

NS_FACETED_END

#endif /* SF1R_CTR_MANAGER_H_ */
