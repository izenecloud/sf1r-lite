#include "CategoryClassifyTable.h"
#include "../util/fcontainer_febird.h"

using namespace sf1r;

namespace
{
const std::string kFileName = "category.bin";
const std::string kEmptyCategory;
}

CategoryClassifyTable::CategoryClassifyTable(
    const std::string& dirPath,
    const std::string& propName)
    : dirPath_(dirPath)
    , propName_(propName)
{
}

bool CategoryClassifyTable::open()
{
    ScopedWriteLock lock(mutex_);

    return load_container_febird(dirPath_, kFileName, categories_);
}

bool CategoryClassifyTable::flush()
{
    ScopedWriteLock lock(mutex_);

    return save_container_febird(dirPath_, kFileName, categories_);
}

void CategoryClassifyTable::resize(std::size_t num)
{
    ScopedWriteLock lock(mutex_);

    categories_.resize(num);
}

void CategoryClassifyTable::setCategory(docid_t docId, const category_t& category)
{
    ScopedWriteLock lock(mutex_);

    categories_[docId] = category;
}

const CategoryClassifyTable::category_t& CategoryClassifyTable::getCategoryHasLock(docid_t docId) const
{
    ScopedReadLock lock(mutex_);

    return getCategoryNoLock(docId);
}

const CategoryClassifyTable::category_t& CategoryClassifyTable::getCategoryNoLock(docid_t docId) const
{
    if (docId < categories_.size())
        return categories_[docId];

    return kEmptyCategory;
}
