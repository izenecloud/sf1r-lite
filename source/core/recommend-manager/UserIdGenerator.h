/**
 * @file UserIdGenerator.h
 * @author Jun Jiang
 * @date 2011-04-19
 */

#ifndef USER_ID_GENERATOR_H
#define USER_ID_GENERATOR_H

#include "RecTypes.h"
#include <ir/id_manager/IDGenerator.h>

#include <string>

namespace sf1r
{

class UserIdGenerator
{
public:
    UserIdGenerator(const std::string& path);

    void flush();

    /**
     * Insert @p strId if not exist.
     * @param[in] strId user string id
     * @return user id converted from @p strId
     */
    userid_t insert(const std::string& strId);

    /**
     * Get @p userId from @p strId.
     * @param[in] strId user string id
     * @param[out] userId conversion result
     * @return true for success, that is, @p strId was inserted before,
     *         false for failure, that is, @p strId does not exist
     */
    bool get(const std::string& strId, userid_t& userId);

private:
    typedef izenelib::ir::idmanager::UniqueIDGenerator<std::string, userid_t, ReadWriteLock> GeneratorType;
    GeneratorType generator_;
};

} // namespace sf1r

#endif // USER_ID_GENERATOR_H
