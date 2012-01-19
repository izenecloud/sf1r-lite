#ifndef SF1R_PRODUCTMANAGER_UUIDGENERATOR_H
#define SF1R_PRODUCTMANAGER_UUIDGENERATOR_H

#include "pm_def.h"
#include "pm_types.h"

#include <common/type_defs.h>

#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/uuid_io.hpp>


namespace sf1r
{

class UuidGenerator
{
public:
    static void Gen(std::string& uuid_str)
    {
        static boost::uuids::uuid uuid_raw;
        Gen_(uuid_raw);
        uuid_str = boost::uuids::to_string(uuid_raw);
    }

    static void Gen(uint128_t& uuid_int)
    {
        Gen_(*reinterpret_cast<boost::uuids::uuid *>(&uuid_int));
    }

    static void Gen(izenelib::util::UString& uuid_ustr)
    {
        static std::string uuid_str;
        Gen(uuid_str, uuid_ustr);
    }

    static void Gen(std::string& uuid_str, izenelib::util::UString& uuid_ustr)
    {
        Gen(uuid_str);
        uuid_ustr.assign(uuid_str, izenelib::util::UString::UTF_8);
    }

private:
    static void Gen_(boost::uuids::uuid& uuid_raw)
    {
        static boost::uuids::random_generator random_gen;
        uuid_raw = random_gen();
    }

};

}

#endif
