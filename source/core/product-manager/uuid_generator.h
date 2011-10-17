#ifndef SF1R_PRODUCTMANAGER_UUIDGENERATOR_H
#define SF1R_PRODUCTMANAGER_UUIDGENERATOR_H

#include <sstream>
#include <common/type_defs.h>
#include "boost_uuid_random_generator.hpp"
#include "boost_uuid_uuid_io.hpp"

#include "pm_def.h"
#include "pm_types.h"


namespace sf1r
{

class UuidGenerator
{
public:
    
    UuidGenerator()
    {
    }
    
    ~UuidGenerator()
    {
    }

    
    std::string GenS(const PMDocumentType& doc)
    {
        boost::uuids::uuid uuid = boost::uuids::random_generator()();
        return boost::uuids::to_string(uuid);
    }
    
    izenelib::util::UString Gen(const PMDocumentType& doc)
    {
        return izenelib::util::UString(GenS(doc), izenelib::util::UString::UTF_8);
    }
    
    
private:

};

}

#endif

