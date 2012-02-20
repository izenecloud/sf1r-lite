/**
 * @file User.h
 * @author Jun Jiang
 * @date 2011-04-18
 */

#ifndef USER_H
#define USER_H

#include "RecTypes.h"
#include <util/ustring/UString.h>

#include <string>
#include <map>

#include <boost/serialization/access.hpp>

namespace sf1r
{

struct User
{
    /** id supplied by SF1 user */
    std::string idStr_;

    /** mapping from property name to UString value */
    typedef std::map<std::string, izenelib::util::UString> PropValueMap;
    PropValueMap propValueMap_;

    template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
        ar & idStr_;
        ar & propValueMap_;
    }
};

} // namespace sf1r

#endif // USER_H
