///
/// @file ontology-rep.h
/// @brief ontology representation class
/// @author Jia Guo <guojia@gmail.com>
/// @date Created 2010-12-09
///

#ifndef SF1R_ONTOLOGY_REP_H_
#define SF1R_ONTOLOGY_REP_H_

#include <boost/serialization/access.hpp>
#include <boost/serialization/serialization.hpp>
#include <3rdparty/msgpack/msgpack.hpp>
#include <common/type_defs.h>
#include "faceted_types.h"
#include "ontology_rep_item.h"
NS_FACETED_BEGIN


/// @brief The memory representation form of a taxonomy.
class OntologyRep
{
public:
    typedef std::list<OntologyRepItem>::iterator item_iterator;
    OntologyRep() {}
    ~OntologyRep() {}

    std::list<OntologyRepItem> item_list;

    item_iterator Begin()
    {
        return item_list.begin();
    }
    
    item_iterator End()
    {
        return item_list.end();
    }
    
    
    bool operator==(const OntologyRep& rep) const
    {
        if (item_list.size()!=rep.item_list.size()) return false;
        std::list<OntologyRepItem>::const_iterator it1 = item_list.begin();
        std::list<OntologyRepItem>::const_iterator it2 = rep.item_list.begin();
        while (it1!=item_list.end() && it2!=rep.item_list.end() )
        {
            if (!(*it1==*it2)) return false;
            ++it1;
            ++it2;
        }
        return true;
    }

    std::string ToString() const
    {
        std::stringstream ss;
        std::list<OntologyRepItem>::const_iterator it = item_list.begin();
        while (it!=item_list.end())
        {
            OntologyRepItem item = *it;
            ++it;
            std::string str;
            item.text.convertString(str, izenelib::util::UString::UTF_8);
            ss<<(int)item.level<<","<<item.id<<","<<str<<","<<item.doc_count<<std::endl;
        }
        return ss.str();
    }

    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
        ar & item_list;
    }

    MSGPACK_DEFINE(item_list);
};
NS_FACETED_END
#endif /* SF1R_ONTOLOGY_REP_H_ */
