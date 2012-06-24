///
/// @file PropValueTable.h
/// @brief a table contains below things for a specific property:
///        1. mapping between property UString value and id
///        2. mapping from doc id to property value id
/// @author Jun Jiang <jun.jiang@izenesoft.com>
/// @date Created 2011-03-24
///

#ifndef SF1R_PROP_VALUE_TABLE_H_
#define SF1R_PROP_VALUE_TABLE_H_

#include <common/inttypes.h>
#include "PropIdTable.h"
#include "../faceted-submanager/faceted_types.h"

#include <util/ustring/UString.h>

#include <boost/memory.hpp>
#include <boost/thread.hpp>

#include <vector>
#include <string>
#include <map>
#include <set>

NS_FACETED_BEGIN

using boost::stl_allocator;
class PropValueTable
{
public:
    /**
     * property value id type.
     * as 0 is reserved as invalid id, meaning no property value is availabe,
     * the valid id range is [1, 2^16) (65535 ids)
     */
    typedef uint16_t pvid_t;

    typedef PropIdTable<pvid_t, uint32_t> ValueIdTable;
    typedef ValueIdTable::PropIdList PropIdList;

    /** mapping from value string to value id */
    typedef std::map<izenelib::util::UString, pvid_t> PropStrMap;
    /** mapping from value id to the map of child values */
    typedef std::vector<PropStrMap> ChildMapTable;

    typedef std::set<pvid_t, std::less<pvid_t>, stl_allocator<pvid_t> > ParentSetType;
    //typedef std::set<pvid_t> ParentSetType;

    typedef boost::shared_mutex MutexType;
    typedef boost::shared_lock<MutexType> ScopedReadLock;
    typedef boost::unique_lock<MutexType> ScopedWriteLock;

    PropValueTable(const std::string& dirPath, const std::string& propName);
    PropValueTable(const PropValueTable& table);

    bool open();
    bool flush();

    const std::string& propName() const { return propName_; }

    std::size_t docIdNum() const { return valueIdTable_.indexTable_.size(); }

    void reserveDocIdNum(std::size_t num);

    void appendPropIdList(const std::vector<pvid_t>& inputIdList);

    std::size_t propValueNum() const { return propStrVec_.size(); }
    void propValueStr(pvid_t pvId, izenelib::util::UString& ustr) const;

    /**
     * Insert property value id.
     * @param path the path of property value, from root node to leaf node
     * @exception MiningException when the new id created is overflow
     * @return value id, if @p path is not inserted before, its new id is created and returned
     */
    pvid_t insertPropValueId(const std::vector<izenelib::util::UString>& path);

    /**
     * Get property value id.
     * @param path the path of property value, from root node to leaf node
     * @return value id, if @p path is not inserted before, 0 is returned
     */
    pvid_t propValueId(const std::vector<izenelib::util::UString>& path) const;

    /**
     * Given value id @p pvId, get its path from root node to leaf node.
     * @param pvId the value id
     * @param path store the path
     */
    void propValuePath(pvid_t pvId, std::vector<izenelib::util::UString>& path) const;

    MutexType& getMutex() const { return mutex_; }

    /**
     * @attention before calling below public functions,
     * you must call this statement for safe concurrent access:
     *
     * <code>
     * PropValueTable::ScopedReadLock lock(PropValueTable::getMutex());
     * </code>
     */
    void getPropIdList(docid_t docId, PropIdList& propIdList) const
    {
        valueIdTable_.getIdList(docId, propIdList);
    }

    const ChildMapTable& childMapTable() const { return childMapTable_; }

    /**
     * Get the root id for @p docId.
     * @param docId the doc id
     * @return the root value id
     */
    pvid_t getRootValueId(docid_t docId) const;

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
     * TODO When boost::memory has a TLS allocator, the tempte could be removed by 
     * assigning a ParentSetType as a member variable of class PropValueTable
     */
    template<typename SetType>
    void parentIdSet(docid_t docId, SetType& parentSet) const;

private:
    /**
     * Save each property value and its parent id to text file for debug use.
     * @param dirPath directory path
     * @param fileName file name
     * @return true for success, false for failure
     */
    bool saveParentId_(const std::string& dirPath, const std::string& fileName) const;

private:
    /** directory path */
    std::string dirPath_;

    /** property name */
    std::string propName_;

    /** mapping from value id to value string */
    std::vector<izenelib::util::UString> propStrVec_;
    /** the number of elements in @c propStrVec_ saved in file */
    unsigned int savePropStrNum_;

    /** mapping from value id to parent value id */
    std::vector<pvid_t> parentIdVec_;
    /** the number of elements in @c parentIdVec_ saved in file */
    unsigned int saveParentIdNum_;

    /** mapping from value id to the map of child values */
    ChildMapTable childMapTable_;

    /** mapping from doc id to a list of property value ids */
    ValueIdTable valueIdTable_;
    /** the number of elements in @c valueIdTable_.indexTable_ saved in file */
    unsigned int saveIndexNum_;
    /** the number of elements in @c valueIdTable_.multiValueTable_ saved in file */
    unsigned int saveValueNum_;

    mutable MutexType mutex_;
};

template<typename SetType>
void PropValueTable::parentIdSet(docid_t docId, SetType& parentSet) const
{
    PropIdList propIdList;
    getPropIdList(docId, propIdList);

    const std::size_t idNum = propIdList.size();
    for (std::size_t i = 0; i < idNum; ++i)
    {
        for (pvid_t pvId = propIdList[i]; pvId; pvId = parentIdVec_[pvId])
        {
            // stop finding parent if already inserted
            if (parentSet.insert(pvId).second == false)
                break;
        }
    }
}

NS_FACETED_END

#endif
