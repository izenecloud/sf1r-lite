/// Some definition for Electronic commerce manager

#ifndef SF1R_EC_ECDEF_H_
#define SF1R_EC_ECDEF_H_


#include <string>
#include <vector>
#include <boost/serialization/access.hpp>
#include <boost/serialization/serialization.hpp>
#include <util/ustring/UString.h>
#include "ec_types.h"

namespace sf1r { namespace ec {

    
    typedef izenelib::util::UString StringType;
    
    struct GroupDocs
    {
        std::vector<uint32_t> doc;
        std::vector<uint32_t> candidate;
        friend class boost::serialization::access;
        template<class Archive>
        void serialize(Archive & ar, const unsigned int version)
        {
            ar & doc & candidate;
        }
    };
    
    struct GroupRes
    {
        friend class boost::serialization::access;
        template<class Archive>
        void serialize(Archive & ar, const unsigned int version)
        {
        }
    };
    
    struct GroupDocsWithRes
    {
        GroupDocs docs;
        //some resource for similarity computation.
        GroupRes res;
        
        friend class boost::serialization::access;
        template<class Archive>
        void serialize(Archive & ar, const unsigned int version)
        {
            ar & docs & res;
        }
    };



}
}

#endif
