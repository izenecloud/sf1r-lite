///
/// @file DateGroupTable.h
/// @brief this table contains date values for each doc id.
/// @author Jun Jiang <jun.jiang@izenesoft.com>
/// @date Created 2012-07-04
///

#ifndef SF1R_DATE_GROUP_TABLE_H_
#define SF1R_DATE_GROUP_TABLE_H_

#include <common/inttypes.h>
#include "PropSharedLock.h"
#include "PropIdTable.h"
#include "../faceted-submanager/faceted_types.h"

#include <string>
#include <set>

NS_FACETED_BEGIN

enum DATE_MASK_TYPE
{
    MASK_ZERO           = 0,
    MASK_YEAR           = 0x7FFE0000,
    MASK_MONTH          =    0x1E000,
    MASK_DAY            =     0x1F00,
    MASK_YEAR_MONTH     = MASK_YEAR | MASK_MONTH,
    MASK_YEAR_MONTH_DAY = MASK_YEAR_MONTH | MASK_DAY
};

class DateGroupTable : public PropSharedLock
{
public:
    /**
     * date value type.
     * its valid range is [0, 2^32)
     */
    typedef uint32_t date_t;

    typedef std::set<date_t> DateSet;

    typedef PropIdTable<date_t, uint32_t> DateValueTable;
    typedef DateValueTable::PropIdList DateValueList;

    DateGroupTable(const std::string& dirPath, const std::string& propName);
    DateGroupTable(const DateGroupTable& table);

    bool open();
    bool flush();

    /**
     * Clear the table to empty.
     */
    void clear();

    const std::string& propName() const { return propName_; }

    std::size_t docIdNum() const { return dateValueTable_.indexTable_.size(); }

    void reserveDocIdNum(std::size_t num);

    void appendDateSet(const DateSet& dateSet);

    /**
     * @attention before calling below public functions,
     * you must call this statement for safe concurrent access:
     *
     * <code>
     * DateGroupTable::ScopedReadLock lock(DateGroupTable::getMutex());
     * </code>
     */
    void getDateSet(docid_t docId, DATE_MASK_TYPE mask, DateSet& dateSet) const;

private:
    /** directory path */
    const std::string dirPath_;

    /** property name */
    const std::string propName_;

    /** mapping from doc id to a list of date values */
    DateValueTable dateValueTable_;

    /** the number of elements in @c dateValueTable_.indexTable_ saved in file */
    unsigned int saveIndexNum_;
    /** the number of elements in @c dateValueTable_.multiValueTable_ saved in file */
    unsigned int saveValueNum_;
};

NS_FACETED_END

#endif // SF1R_DATE_GROUP_TABLE_H_
