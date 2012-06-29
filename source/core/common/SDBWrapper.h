/**
 * @file SDBWrapper.h
 * @brief a wrapper of SDB class
 * @author Jun Jiang
 * @date 2012-06-20
 */

#ifndef SF1R_SDB_WRAPPER_H
#define SF1R_SDB_WRAPPER_H

#include <sdb/SequentialDB.h>
#include <string>
#include <glog/logging.h>

namespace sf1r
{

template <typename KeyType = std::string,
          typename ValueType = izenelib::am::NullType,
          typename LockType = izenelib::util::ReadWriteLock,
          template <typename, typename, typename> class ContainerType =
              izenelib::am::tc_hash>
class SDBWrapper
{
public:
    typedef KeyType key_type;
    typedef ValueType value_type;
    typedef LockType lock_type;

    /**
     * @param path the file path
     */
    SDBWrapper(const std::string& path);

    /** Flush to disk storage. */
    void flush();

    /**
     * Get the value mapped to @p key.
     * @param[in] key the key
     * @param[out] value the value
     * @return true for success, false for failure
     * @note if @p key is not found, it returns true and
     *       @p value would not be touched.
     */
    bool get(const key_type& key, value_type& value);

    /**
     * Update the value mapped to @p key.
     * @param[in] key the key
     * @param[in] value the value to update
     * @return true for success, false for failure
     */
    bool update(const key_type& key, const value_type& value);

private:
    typedef ContainerType<key_type, value_type, lock_type> container_type;
    typedef izenelib::sdb::SequentialDB<key_type, value_type,
                                        lock_type, container_type> sdb_type;

    sdb_type db_;
};

template <typename KeyType, typename ValueType, typename LockType,
          template <typename, typename, typename> class ContainerType>
SDBWrapper<KeyType, ValueType, LockType, ContainerType>::SDBWrapper(const std::string& path)
    : db_(path)
{
    db_.open();
}

template <typename KeyType, typename ValueType, typename LockType,
          template <typename, typename, typename> class ContainerType>
void SDBWrapper<KeyType, ValueType, LockType, ContainerType>::flush()
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

template <typename KeyType, typename ValueType, typename LockType,
          template <typename, typename, typename> class ContainerType>
bool SDBWrapper<KeyType, ValueType, LockType, ContainerType>::get(const key_type& key, value_type& value)
{
    bool result = false;

    try
    {
        db_.getValue(key, value);
        result = true;
    }
    catch(izenelib::util::IZENELIBException& e)
    {
        LOG(ERROR) << "exception in SDB::getValue(): " << e.what();
    }

    return result;
}

template <typename KeyType, typename ValueType, typename LockType,
          template <typename, typename, typename> class ContainerType>
bool SDBWrapper<KeyType, ValueType, LockType, ContainerType>::update(const key_type& key, const value_type& value)
{
    bool result = false;

    try
    {
        result = db_.update(key, value);
    }
    catch(izenelib::util::IZENELIBException& e)
    {
        LOG(ERROR) << "exception in SDB::update(): " << e.what();
    }

    return result;
}

} // namespace sf1r

#endif // SF1R_SDB_WRAPPER_H
