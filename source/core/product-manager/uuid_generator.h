#ifndef SF1R_PRODUCTMANAGER_UUIDGENERATOR_H
#define SF1R_PRODUCTMANAGER_UUIDGENERATOR_H

#include <sstream>
#include <common/type_defs.h>
#include <3rdparty/boost/uuid/random_generator.hpp>
#include <3rdparty/boost/uuid/uuid_io.hpp>

#include "pm_def.h"
#include "pm_types.h"


namespace sf1r
{

void generateUUID(std::string& uuid_str, const PMDocumentType& doc)
{
    static boost::uuids::random_generator random_gen;
    uuid_str = boost::uuids::to_string(random_gen());
}

void generateUUID(izenelib::util::UString& uuid_ustr, const PMDocumentType& doc)
{
    std::string uuid_str;
    generateUUID(uuid_str, doc);
    uuid_ustr.assign(uuid_str, izenelib::util::UString::UTF_8);
}

}

#endif
