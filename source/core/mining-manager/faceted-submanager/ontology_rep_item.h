///
/// @file ontology-rep-item.h
/// @brief item used for ontology representation class
/// @author Jia Guo <guojia@gmail.com>
/// @date Created 2010-12-09
///

#ifndef SF1R_ONTOLOGY_REP_ITEM_H_
#define SF1R_ONTOLOGY_REP_ITEM_H_


#include <boost/serialization/access.hpp>
#include <boost/serialization/serialization.hpp>
#include <common/type_defs.h>
#include "faceted_types.h"

NS_FACETED_BEGIN


/// @brief The memory representation form of a taxonomy.
class OntologyRepItem {
public:
    OntologyRepItem() {}
    OntologyRepItem(uint8_t plevel, const CategoryNameType& ptext, CategoryIdType pid, uint32_t pdoc_count)
    :level(plevel), text(ptext), id(pid), doc_count(pdoc_count)
    {
    }
    uint8_t level;
    CategoryNameType text;
    CategoryIdType id;
    uint32_t doc_count;
    
    bool operator==(const OntologyRepItem& item) const
    {
      return level==item.level&&text==item.text&&id==item.id&&doc_count==item.doc_count;
    }
    
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
        ar & level & text & id & doc_count;
    }

};
NS_FACETED_END
#endif /* SF1R_ONTOLOGY_REP_ITEM_H_ */
