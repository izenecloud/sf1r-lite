///
/// @file prop_value_table.h
/// @brief a table contains below things for a specific property:
///        1. mapping between property UString value and id
///        2. mapping from doc id to property value id
/// @author Jun Jiang <jun.jiang@izenesoft.com>
/// @date Created 2011-03-24
///

#ifndef SF1R_PROP_VALUE_TABLE_H_
#define SF1R_PROP_VALUE_TABLE_H_

#include <common/inttypes.h>
#include <util/ustring/UString.h>
#include "faceted_types.h"
#include "ontology_rep.h"

#include <vector>
#include <string>
#include <map>
#include <set>

NS_FACETED_BEGIN

class PropValueTable
{
public:
    /**
     * property value id type.
     * as 0 is reserved as invalid id, meaning no property value is availabe,
     * the valid id range is [1, 2^16) (65535 ids)
     */
    typedef uint16_t pvid_t;

    /** a list of property value id */
    typedef std::vector<pvid_t> ValueIdList;

    /** mapping from doc id to a list of property value id */
    typedef std::vector<ValueIdList> ValueIdTable;

    PropValueTable(
        const std::string& dirPath,
        const std::string& propName
    );

    bool open();

    bool flush();

    const char* propName() const {
        return propName_.c_str();
    }

    ValueIdTable& valueIdTable() {
        return valueIdTable_;
    }

    const ValueIdTable& valueIdTable() const {
        return valueIdTable_;
    }

    const izenelib::util::UString& propValueStr(pvid_t pvId) const {
        return propStrVec_[pvId];
    }

    std::size_t propValueNum() const {
        return propStrVec_.size();
    }

    /**
     * Get property value id.
     * @param value property string
     * @exception MiningException when the new id created is overflow
     * @return value id, if @p value is not inserted before, its new id is created and returned
     */
    pvid_t propValueId(const izenelib::util::UString& value);

    /**
     * Get property value id.
     * @param value property string
     * @return value id, if @p value is not inserted before, 0 is returned
     */
    pvid_t propValueId(const izenelib::util::UString& value) const {
        pvid_t pvId = 0;

        std::map<izenelib::util::UString, pvid_t>::const_iterator it = propStrMap_.find(value);
        if (it != propStrMap_.end())
        {
            pvId = it->second; 
        }

        return pvId;
    }

    /**
     * Set the value tree, it defines the hierachical levels for values,
     * so that the group values could be returned in the form of this tree.
     * @param valueTree the value tree
     * @return true for success, false for error in parsing @p valueTree
     * @attention before calling this function, @c open() must be called to load all values.
     */
    bool setValueTree(const faceted::OntologyRep& valueTree);

    const faceted::OntologyRep& valueTree() const {
        return valueTree_;
    }

    /**
     * Whether @p docId belongs to group label of @p labelId.
     * @param docId the doc id
     * @param labelId the property value id of group label
     * @return true for belongs to, false for not belongs to.
     */
    bool testDoc(docid_t docId, pvid_t labelId) const;

    /**
     * Get value ids of @p docId, including all its parent ids.
     * @param docId the doc id
     * @param parentSet the set of value ids
     */
    void parentIdSet(docid_t docId, std::set<pvid_t>& parentSet) const;

private:
    /** directory path */
    std::string dirPath_;

    /** property name */
    std::string propName_;

    /** mapping from property value string to value id */
    std::map<izenelib::util::UString, pvid_t> propStrMap_;

    /** mapping from property value id to value string */
    std::vector<izenelib::util::UString> propStrVec_;
    /** the number of value strings saved in file */
    unsigned int savePropStrNum_;

    /** mapping from doc id to a list of property value id */
    ValueIdTable valueIdTable_;
    /** the number of elements in @c valueIdTable_ saved in file */
    unsigned int saveDocIdNum_;

    /** the values configured in hierachical levels */
    faceted::OntologyRep valueTree_;

    /** mapping from value id to parent value id */
    std::vector<pvid_t> parentIdVec_;
};

NS_FACETED_END

#endif 
