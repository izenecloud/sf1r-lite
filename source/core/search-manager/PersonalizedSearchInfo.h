/**
 * @file PersonalizedSearchInfo.h
 * @author Zhongxia Li
 * @date Apr 29, 2011
 * @brief Wrap up Personalized Search Info
 */
#ifndef PERSONALIZED_SEARCH_INFO_H
#define PERSONALIZED_SEARCH_INFO_H

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

struct PersonalSearchInfo
{
    User user;
    bool enabled;
};

}

#endif /* PERSONALIZED_SEARCH_INFO_H */
