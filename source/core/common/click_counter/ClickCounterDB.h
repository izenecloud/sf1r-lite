///
/// @file ClickCounterDB.h
/// @brief a db storage, it stores ClickCounter instance for each key
/// @author Jun Jiang <jun.jiang@izenesoft.com>
/// @date Created 2012-03-15
///

#ifndef SF1R_CLICK_COUNTER_DB_H
#define SF1R_CLICK_COUNTER_DB_H

#include "ClickCounter.h"
#include <sdb/SequentialDB.h>

#include <string>
#include <glog/logging.h>

namespace sf1r
{

template <typename KeyType = std::string,
          typename ValueType = int,
          typename FreqType = int>
class ClickCounterDB
{
public:
    typedef KeyType key_type;
    typedef ClickCounter<ValueType, FreqType> click_counter_type;

    /**
     * @param path the file path
     */
    ClickCounterDB(const std::string& path);

    /** Flush to disk storage. */
    void flush();

    /**
     * Get ClickCounter mapped to @p key.
     * @param[in] key the key value
     * @param[out] clickCounter the ClickCounter instance
     * @return true for success, false for failure
     */
    bool get(const key_type& key, click_counter_type& clickCounter);

    /**
     * Update ClickCounter mapped to @p key.
     * @param[in] key the key value
     * @param[in] clickCounter the ClickCounter to update
     * @return true for success, false for failure
     */
    bool update(const key_type& key, const click_counter_type& clickCounter);

private:
    typedef izenelib::sdb::unordered_sdb_tc<key_type, click_counter_type, ReadWriteLock> db_type;

    db_type db_;
};

template <typename KeyType, typename ValueType, typename FreqType>
ClickCounterDB<KeyType, ValueType, FreqType>::ClickCounterDB(const std::string& path)
    : db_(path)
{
    db_.open();
}

template <typename KeyType, typename ValueType, typename FreqType>
void ClickCounterDB<KeyType, ValueType, FreqType>::flush()
{
    try
    {
        db_.flush();
    }
    catch(izenelib::util::IZENELIBException& e)
    {
        LOG(ERROR) << "exception in SDB::flush(): " << e.what();
    }
}

template <typename KeyType, typename ValueType, typename FreqType>
bool ClickCounterDB<KeyType, ValueType, FreqType>::get(const key_type& key, click_counter_type& clickCounter)
{
    bool result = false;

    try
    {
        db_.getValue(key, clickCounter);
        result = true;
    }
    catch(izenelib::util::IZENELIBException& e)
    {
        LOG(ERROR) << "exception in SDB::getValue(): " << e.what();
    }

    return result;
}

template <typename KeyType, typename ValueType, typename FreqType>
bool ClickCounterDB<KeyType, ValueType, FreqType>::update(const key_type& key, const click_counter_type& clickCounter)
{
    bool result = false;

    try
    {
        result = db_.update(key, clickCounter);
    }
    catch(izenelib::util::IZENELIBException& e)
    {
        LOG(ERROR) << "exception in SDB::update(): " << e.what();
    }

    return result;
}

} // namespace sf1r

#endif // SF1R_CLICK_COUNTER_DB_H
