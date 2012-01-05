#ifndef SF1R_PRODUCTMANAGER_UUIDGENERATOR_H
#define SF1R_PRODUCTMANAGER_UUIDGENERATOR_H

#include "pm_def.h"
#include "pm_types.h"

#include <common/type_defs.h>
#include <3rdparty/boost/uuid/random_generator.hpp>
#include <3rdparty/boost/uuid/uuid_io.hpp>


namespace sf1r
{

class UuidGenerator
{
public:
    static void Gen(std::string& uuid_str)
    {
        static boost::uuids::random_generator random_gen;
        uuid_str = boost::uuids::to_string(random_gen());
    }

    static void Gen(izenelib::util::UString& uuid_ustr)
    {
        std::string uuid_str;
        Gen(uuid_str);
        uuid_ustr.assign(uuid_str, izenelib::util::UString::UTF_8);
    }

};

}

#endif
