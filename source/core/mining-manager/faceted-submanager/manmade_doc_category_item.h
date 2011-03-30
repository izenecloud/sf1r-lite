///
/// @file manmade_doc_category_item.h
/// @brief item used for manmade doc category
/// @author Jia Guo <guojia@gmail.com>
/// @date Created 2010-12-30
///

#ifndef SF1R_MANMADE_DOC_CATEGORY_ITEM_H_
#define SF1R_MANMADE_DOC_CATEGORY_ITEM_H_


#include <boost/serialization/access.hpp>
#include <boost/serialization/serialization.hpp>
#include <common/type_defs.h>
#include "faceted_types.h"

NS_FACETED_BEGIN


/// @brief The memory representation form of a taxonomy.
class ManmadeDocCategoryItem
{
public:
    ManmadeDocCategoryItem():docid(0), cid(0)
    {
    }
    ManmadeDocCategoryItem(uint32_t pdocid, const izenelib::util::UString& pstr_docid, const CategoryNameType& pcname)
            :docid(pdocid), str_docid(pstr_docid), cname(pcname)
    {
    }
    uint32_t docid;
    izenelib::util::UString str_docid;
    CategoryIdType cid;
    CategoryNameType cname;

    bool ValidInput() const
    {
        if (docid>0&&str_docid.length()>0&&cid>0&&cname.length()>0)
        {
            return true;
        }
        return false;
    }

    static bool CompareStrDocId(const ManmadeDocCategoryItem& item1, const ManmadeDocCategoryItem& item2)
    {
        return item1.str_docid<item2.str_docid;
    }

//     bool operator==(const OntologyRepItem& item) const
//     {
//       return level==item.level&&text==item.text&&id==item.id&&doc_count==item.doc_count;
//     }

    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
        ar & docid & str_docid & cid & cname;
    }

};
NS_FACETED_END
#endif
