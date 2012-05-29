///
/// @file AttrTable.h
/// @brief a table contains below things for all attribute name and values:
///        1. mapping between attribute UString name and id
///        2. mapping between attribute UString value and id
///        3. mapping from doc id to a list of attribute value id
/// @author Jun Jiang <jun.jiang@izenesoft.com>
/// @date Created 2011-06-22
///

#ifndef SF1R_ATTR_TABLE_H_
#define SF1R_ATTR_TABLE_H_

#include <common/inttypes.h>
#include <util/ustring/UString.h>
#include "../faceted-submanager/faceted_types.h"

#include <vector>
#include <string>
#include <map>

NS_FACETED_BEGIN

class AttrTable
{
public:
    /**
     * attribute name id type.
     * as 0 is reserved as invalid id, meaning no attribute name is availabe,
     * the valid id range is [1, 2^32) (4G ids)
     */
    typedef uint32_t nid_t;

    /**
     * attribute value id type.
     * as 0 is reserved as invalid id, meaning no attribute value is availabe,
     * the valid id range is [1, 2^32) (4G ids)
     */
    typedef uint32_t vid_t;

    /** a list of attribute value id */
    typedef std::vector<vid_t> ValueIdList;

    /** mapping from doc id to a list of attribute value id */
    typedef std::vector<ValueIdList> ValueIdTable;

    AttrTable();

    bool open(
        const std::string& dirPath,
        const std::string& propName
    );

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

    const izenelib::util::UString& nameStr(nid_t nameId) const {
        return nameStrVec_[nameId];
    }

    const izenelib::util::UString& valueStr(vid_t valueId) const {
        return valueStrVec_[valueId];
    }

    std::size_t nameNum() const {
        return nameStrVec_.size();
    }

    std::size_t valueNum() const {
        return valueStrVec_.size();
    }

    /**
     * Get attribute name id.
     * @param name attribute name string
     * @exception MiningException when the new id created is overflow
     * @return name id, if @p name is not inserted before, its new id is created and returned
     */
    nid_t nameId(const izenelib::util::UString& name);

    /**
     * Get attribute name id.
     * @param name attribute name string
     * @return name id, if @p name is not inserted before, 0 is returned
     */
    nid_t nameId(const izenelib::util::UString& name) const {
        nid_t nameId = 0;

        std::map<izenelib::util::UString, nid_t>::const_iterator it = nameStrMap_.find(name);
        if (it != nameStrMap_.end())
        {
            nameId = it->second; 
        }

        return nameId;
    }

    /**
     * Get attribute value id.
     * @param nameId the attribute name id
     * @param value attribute value string
     * @exception MiningException when the new id created is overflow
     * @return value id, if @p value is not inserted before, its new id is created and returned
     * @note as @c ValueIdTable only stores attribute value id,
     * to differentiate their attribute names for those same attribute value,
     * different @p nameId must return different value id
     */
    vid_t valueId(nid_t nameId, const izenelib::util::UString& value);

    /**
     * Get attribute value id.
     * @param nameId the attribute name id
     * @param value attribute value string
     * @return value id, if @p value is not inserted before, 0 is returned
     */
    vid_t valueId(nid_t nameId, const izenelib::util::UString& value) const {
        assert(nameId < valueStrTable_.size());

        vid_t valueId = 0;
        const ValueStrMap& valueStrMap = valueStrTable_[nameId];

        ValueStrMap::const_iterator it = valueStrMap.find(value);
        if (it != valueStrMap.end())
        {
            valueId = it->second;
        }

        return valueId;
    }

    /**
     * Get the name id of value id @p valueId.
     * @param valueId the value id
     * @return the name id
     */
    nid_t valueId2NameId(vid_t valueId) const {
        assert(valueId < nameIdVec_.size());
        return nameIdVec_[valueId];
    }

private:
    /** directory path */
    std::string dirPath_;

    /** property name */
    std::string propName_;

    /** mapping from attribute name id to name string */
    std::vector<izenelib::util::UString> nameStrVec_;
    /** the number of elements in @c nameStrVec_ saved in file */
    unsigned int saveNameStrNum_;
    /** mapping from attribute name string to name id */
    std::map<izenelib::util::UString, nid_t> nameStrMap_;

    /** mapping from attribute value id to value string */
    std::vector<izenelib::util::UString> valueStrVec_;
    /** the number of elements in @c valueStrVec_ saved in file */
    unsigned int saveValueStrNum_;

    /** mapping from value id to name id */
    std::vector<nid_t> nameIdVec_;
    /** the number of elements in @c nameIdVec_ saved in file */
    unsigned int saveNameIdNum_;

    /** mapping from attribute value string to value id */
    typedef std::map<izenelib::util::UString, vid_t> ValueStrMap;
    /** mapping from attribute name id to @c ValueStrMap */
    std::vector<ValueStrMap> valueStrTable_;

    /** mapping from doc id to a list of attribute value id */
    ValueIdTable valueIdTable_;
    /** the number of elements in @c valueIdTable_ saved in file */
    unsigned int saveDocIdNum_;
};

NS_FACETED_END

#endif //SF1R_ATTR_TABLE_H_
