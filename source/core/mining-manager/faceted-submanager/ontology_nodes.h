///
/// @file ontology-nodes.h
/// @brief ontology nodes storage class
/// @author Jia Guo <guojia@gmail.com>
/// @date Created 2010-12-09
///

#ifndef SF1R_ONTOLOGY_NODES_H_
#define SF1R_ONTOLOGY_NODES_H_

#include <boost/serialization/access.hpp>
#include <boost/serialization/serialization.hpp>
#include <common/type_defs.h>
#include "faceted_types.h"
#include "ontology_node_value.h"
NS_FACETED_BEGIN


class OntologyNodes
{


public:
    typedef std::list<CategoryIdType> ChildrenType;
    OntologyNodes();
    ~OntologyNodes();

    bool Open(const std::string& file);

//   bool CopyFrom(const std::string& from_dir, const std::string& to_dir);

    bool AddCategoryNode(CategoryIdType parent_id, const CategoryNameType& name, CategoryIdType& id);



    bool RenameCategory(CategoryIdType id, const CategoryNameType& new_name);

    bool MoveCategory(CategoryIdType id, CategoryIdType new_parent_id);

    bool DelCategory(CategoryIdType id);

    bool GetChildren(CategoryIdType parent_id, ChildrenType& children) const;

    bool GetParent(CategoryIdType id, CategoryIdType& parent_id) const;

    bool GetCategoryName(CategoryIdType id, CategoryNameType& name) const;

    bool GetCategoryRule(CategoryIdType id, OntologyNodeRule& rule) const;

    bool SetCategoryRule(CategoryIdType id, const OntologyNodeRule& rule);

    bool Save();

    inline static CategoryIdType TopCategoryId()
    {
        return 0;
    }

    //for test propose
    bool AddCategoryNodeStr(CategoryIdType parent_id, const std::string& name, CategoryIdType& id);

    bool RenameCategoryStr(CategoryIdType id, const std::string& new_name);

    bool GetCategoryNameStr(CategoryIdType id, std::string& name) const;


    static CategoryNameType GetTopName()
    {
        return izenelib::util::UString("__top", izenelib::util::UString::UTF_8);
    }

    uint32_t GetCidCount() const
    {
        return nodes_.size();
    }

private:

    bool IsValidId(CategoryIdType id, bool valid_top = false) const;


private:
    std::string file_;
    std::list<CategoryIdType> available_id_list_;
    std::set<CategoryIdType> available_id_map_;
    std::vector<OntologyNodeValue> nodes_;

};
NS_FACETED_END
#endif
