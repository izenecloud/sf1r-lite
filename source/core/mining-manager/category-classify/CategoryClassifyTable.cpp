#include "CategoryClassifyTable.h"
#include "../util/fcontainer_febird.h"
#include <fstream>

using namespace sf1r;

namespace
{
const std::string kBinaryFileName = "category.bin";
const std::string kTextFileName = "category.txt";
const CategoryClassifyTable::category_rflag_t kEmptyCategory;
}

CategoryClassifyTable::CategoryClassifyTable(
    const std::string& dirPath,
    const std::string& propName,
    bool isDebug)
    : dirPath_(dirPath)
    , propName_(propName)
    , categories_(1) // doc id 0 is reserved for an empty doc
    , isDebug_(isDebug)
{
}

bool CategoryClassifyTable::open()
{
    ScopedWriteLock lock(mutex_);

    return load_container_febird(dirPath_, kBinaryFileName, categories_);
}

bool CategoryClassifyTable::flush()
{
    ScopedWriteLock lock(mutex_);

    return save_container_febird(dirPath_, kBinaryFileName, categories_) &&
        saveTextFile_();
}

bool CategoryClassifyTable::saveTextFile_() const
{
    if (!isDebug_)
        return true;

    boost::filesystem::path filePath(dirPath_);
    filePath /= kTextFileName;
    std::string pathStr = filePath.string();

    LOG(INFO) << "saving file: " << kTextFileName
              << ", element num: " << categories_.size();

    std::ofstream ofs(pathStr.c_str());
    if (! ofs)
    {
        LOG(ERROR) << "failed opening file " << pathStr;
        return false;
    }

    for (std::size_t docId = 1; docId < categories_.size(); ++docId)
    {
        const category_rflag_t& categoryRFlag = categories_[docId];
        if (categoryRFlag.first.empty())
            continue;

        ofs << docId << "\t"
            << categoryRFlag.first << "\t"
            << categoryRFlag.second << std::endl;
    }

    return true;
}

void CategoryClassifyTable::resize(std::size_t num)
{
    ScopedWriteLock lock(mutex_);

    categories_.resize(num);
}

void CategoryClassifyTable::setCategory(docid_t docId, const category_t& category, bool ruleFlag)
{
    ScopedWriteLock lock(mutex_);

    categories_[docId].first = category;
    categories_[docId].second = ruleFlag;
}

const CategoryClassifyTable::category_rflag_t& CategoryClassifyTable::getCategoryHasLock(docid_t docId) const
{
    ScopedReadLock lock(mutex_);

    return getCategoryNoLock(docId);
}

const CategoryClassifyTable::category_rflag_t& CategoryClassifyTable::getCategoryNoLock(docid_t docId) const
{
    if (docId < categories_.size())
        return categories_[docId];

    return kEmptyCategory;
}
