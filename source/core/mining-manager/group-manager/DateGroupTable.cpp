#include "DateGroupTable.h"
#include <mining-manager/util/fcontainer_febird.h>
#include <mining-manager/MiningException.hpp>

#include <glog/logging.h>
#include <algorithm> // swap

namespace
{
const char* SUFFIX_INDEX_ID = ".index_id.bin";
const char* SUFFIX_VALUE_ID = ".value_id.bin";
}

NS_FACETED_BEGIN

DateGroupTable::DateGroupTable()
    : saveIndexNum_(0)
    , saveValueNum_(0)
{
}

DateGroupTable::DateGroupTable(const std::string& dirPath, const std::string& propName)
    : dirPath_(dirPath)
    , propName_(propName)
    , saveIndexNum_(0)
    , saveValueNum_(0)
{
}

DateGroupTable::DateGroupTable(const DateGroupTable& table)
    : dirPath_(table.dirPath_)
    , propName_(table.propName_)
    , dateValueTable_(table.dateValueTable_)
    , saveIndexNum_(table.saveIndexNum_)
    , saveValueNum_(table.saveValueNum_)
{
}

DateGroupTable& DateGroupTable::operator=(const DateGroupTable& other)
{
    if (this != &other)
    {
        ScopedWriteLock lock(mutex_);
        ScopedReadLock otherLock(other.mutex_);

        dirPath_ = other.dirPath_;
        propName_ = other.propName_;
        dateValueTable_ = other.dateValueTable_;
        saveIndexNum_ = other.saveIndexNum_;
        saveValueNum_ = other.saveValueNum_;
    }

    return *this;
}

void DateGroupTable::swap(DateGroupTable& other)
{
    if (this == &other)
        return;

    ScopedWriteLock lock(mutex_);
    ScopedWriteLock otherLock(other.mutex_);

    dirPath_.swap(other.dirPath_);
    propName_.swap(other.propName_);
    dateValueTable_.swap(other.dateValueTable_);
    std::swap(saveIndexNum_, other.saveIndexNum_);
    std::swap(saveValueNum_, other.saveValueNum_);
}

bool DateGroupTable::open()
{
    ScopedWriteLock lock(mutex_);

    return load_container_febird(dirPath_, propName_ + SUFFIX_INDEX_ID, dateValueTable_.indexTable_, saveIndexNum_) &&
           load_container_febird(dirPath_, propName_ + SUFFIX_VALUE_ID, dateValueTable_.multiValueTable_, saveValueNum_);
}

bool DateGroupTable::flush()
{
    ScopedReadLock lock(mutex_);

    return save_container_febird(dirPath_, propName_ + SUFFIX_INDEX_ID, dateValueTable_.indexTable_, saveIndexNum_) &&
           save_container_febird(dirPath_, propName_ + SUFFIX_VALUE_ID, dateValueTable_.multiValueTable_, saveValueNum_);
}

void DateGroupTable::clear()
{
    ScopedWriteLock lock(mutex_);

    dateValueTable_.clear();
    saveIndexNum_ = 0;
    saveValueNum_ = 0;
}

void DateGroupTable::resize(std::size_t num)
{
    ScopedWriteLock lock(mutex_);

    dateValueTable_.resize(num);
}

void DateGroupTable::setDateSet(docid_t docId, const DateSet& dateSet)
{
    ScopedWriteLock lock(mutex_);

    dateValueTable_.setIdList(docId, dateSet);
}

void DateGroupTable::getDateSet(docid_t docId, DATE_MASK_TYPE mask, DateSet& dateSet) const
{
    DateValueList dateList;
    dateValueTable_.getIdList(docId, dateList);

    dateSet.clear();
    const std::size_t dateNum = dateList.size();

    for (std::size_t i = 0; i < dateNum; ++i)
    {
        date_t date = dateList[i] & mask;
        dateSet.insert(date);
    }
}

NS_FACETED_END
