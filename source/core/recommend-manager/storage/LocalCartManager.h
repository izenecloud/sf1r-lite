/**
 * @file LocalCartManager.h
 * @author Jun Jiang
 * @date 2012-02-01
 */

#ifndef LOCAL_CART_MANAGER_H
#define LOCAL_CART_MANAGER_H

#include "CartManager.h"

#include <sdb/SequentialDB.h>
#include <boost/serialization/vector.hpp> // serialize std::vector

namespace sf1r
{

class LocalCartManager : public CartManager
{
public:
    LocalCartManager(const std::string& path);

    virtual void flush();

    virtual bool updateCart(
        const std::string& userId,
        const std::vector<itemid_t>& itemVec
    );

    virtual bool getCart(
        const std::string& userId,
        std::vector<itemid_t>& itemVec
    );

private:
    typedef izenelib::sdb::unordered_sdb_tc<std::string, std::vector<itemid_t>,
                                            ReadWriteLock> SDBType;
    SDBType container_;
};

} // namespace sf1r

#endif // LOCAL_CART_MANAGER_H
