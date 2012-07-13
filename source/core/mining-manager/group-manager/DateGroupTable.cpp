#include "DateGroupTable.h"
#include <mining-manager/util/fcontainer_febird.h>
#include <mining-manager/MiningException.hpp>

#include <glog/logging.h>

namespace
{
const char* SUFFIX_INDEX_ID = ".index_id.bin";
const char* SUFFIX_VALUE_ID = ".value_id.bin";
}

NS_FACETED_BEGIN

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

bool DateGroupTable::open()
{
    ScopedWriteLock lock(mutex_);

    return load_container_febird(dirPath_, propName_ + SUFFIX_INDEX_ID, dateValueTable_.indexTable_, saveIndexNum_) &&
           load_container_febird(dirPath_, propName_ + SUFFIX_VALUE_ID, dateValueTable_.multiValueTable_, saveValueNum_);
}

bool DateGroupTable::flush()
{
    ScopedWriteLock lock(mutex_);

    return save_container_febird(dirPath_, propName_ + SUFFIX_INDEX_ID, dateValueTable_.indexTable_, saveIndexNum_) &&
           save_container_febird(dirPath_, propName_ + SUFFIX_VALUE_ID, dateValueTable_.multiValueTable_, saveValueNum_);
}

void DateGroupTable::reserveDocIdNum(std::size_t num)
{
    ScopedWriteLock lock(mutex_);

    dateValueTable_.indexTable_.reserve(num);
}

void DateGroupTable::appendDateSet(const DateSet& dateSet)
{
    ScopedWriteLock lock(mutex_);

    dateValueTable_.appendIdList(dateSet);
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
