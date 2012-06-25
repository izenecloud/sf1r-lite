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
#include "../group-manager/PropIdTable.h"
#include "../group-manager/PropSharedLock.h"

#include <vector>
#include <string>
#include <map>

NS_FACETED_BEGIN

class AttrTable : public PropSharedLock
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
     * and as the most significant bit is used as index flag in PropIdTable,
     * the valid id range is [1, 2^31) (2G ids)
     */
    typedef uint32_t vid_t;

    typedef PropIdTable<vid_t, uint32_t> ValueIdTable;
    typedef ValueIdTable::PropIdList ValueIdList;

    AttrTable();

    bool open(
        const std::string& dirPath,
        const std::string& propName
    );

    bool flush();

    const char* propName() const { return propName_.c_str(); }

    std::size_t nameNum() const { return nameStrVec_.size(); }

    std::size_t valueNum() const { return valueStrVec_.size(); }

    std::size_t docIdNum() const { return valueIdTable_.indexTable_.size(); }

    void reserveDocIdNum(std::size_t num);

    void appendValueIdList(const std::vector<vid_t>& inputIdList);

    /**
     * Insert attribute name id.
     * @param name attribute name string
     * @exception MiningException when the new id created is overflow
     * @return name id, if @p name is not inserted before, its new id is created and returned
     */
    nid_t insertNameId(const izenelib::util::UString& name);

    /**
     * Get attribute name id.
     * @param name attribute name string
     * @return name id, if @p name is not inserted before, 0 is returned
     */
    nid_t nameId(const izenelib::util::UString& name) const;

    /**
     * Insert attribute value id.
     * @param nameId the attribute name id
     * @param value attribute value string
     * @exception MiningException when the new id created is overflow
     * @return value id, if @p value is not inserted before, its new id is created and returned
     * @note as @c ValueIdTable only stores attribute value id,
     * to differentiate their attribute names for those same attribute value,
     * different @p nameId must return different value id
     */
    vid_t insertValueId(nid_t nameId, const izenelib::util::UString& value);

    /**
     * Get attribute value id.
     * @param nameId the attribute name id
     * @param value attribute value string
     * @return value id, if @p value is not inserted before, 0 is returned
     */
    vid_t valueId(nid_t nameId, const izenelib::util::UString& value) const;

    /**
     * @attention before calling below public functions,
     * you must call this statement for safe concurrent access:
     *
     * <code>
     * AttrTable::ScopedReadLock lock(AttrTable::getMutex());
     * </code>
     */
    void getValueIdList(docid_t docId, ValueIdList& valueIdList) const
    {
        valueIdTable_.getIdList(docId, valueIdList);
    }

    const izenelib::util::UString& nameStr(nid_t nameId) const { return nameStrVec_[nameId]; }

    const izenelib::util::UString& valueStr(vid_t valueId) const { return valueStrVec_[valueId]; }

    /**
     * Get the name id of value id @p valueId.
     * @param valueId the value id
     * @return the name id
     */
    nid_t valueId2NameId(vid_t valueId) const { return nameIdVec_[valueId]; }

private:
    /**
     * Save each name/value pair to text file for debug use.
     * @param dirPath directory path
     * @param fileName file name
     * @return true for success, false for failure
     */
    bool saveNameValuePair_(const std::string& dirPath, const std::string& fileName) const;

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
    /** the number of elements in @c valueIdTable_.indexTable_ saved in file */
    unsigned int saveIndexNum_;
    /** the number of elements in @c valueIdTable_.multiValueTable_ saved in file */
    unsigned int saveValueNum_;
};

NS_FACETED_END

#endif //SF1R_ATTR_TABLE_H_
