#include "ctr_manager.h"

NS_FACETED_BEGIN

const std::string CTRManager::kCtrPropName("_ctr");

CTRManager::CTRManager(const std::string& dirPath, size_t docNum)
    : docNum_(docNum)
    , docClickCountList_(new NumericPropertyTable<count_t>(INT32_PROPERTY_TYPE))
    , db_(NULL)
{
    filePath_ = dirPath + "/ctr.db";

    // docid start from 1
    docClickCountList_->resize(docNum + 1);
}

CTRManager::~CTRManager()
{
    if (db_)
    {
        db_->close();
        delete db_;
    }
}

bool CTRManager::open()
{
    ScopedWriteLock lock(mutex_);

    db_ = new DBType(filePath_);

    if (!db_ || !db_->open())
    {
        return false;
    }

    size_t nums = db_->num_items();
    if (nums > 0)
    {
        docid_t k;
        count_t v;

        DBType::SDBCursor locn = db_->get_first_locn();
        while (db_->get(locn, k, v))
        {
            docClickCountList_->setValue(k, v);

            db_->seq(locn);
        }
    }

    return true;
}

void CTRManager::updateDocNum(size_t docNum)
{
    ScopedWriteLock lock(mutex_);

    if (docNum_ != docNum)
    {
        docNum_ = docNum;
        docClickCountList_->resize(docNum + 1);
    }
}

bool CTRManager::update(docid_t docId)
{
    ScopedWriteLock lock(mutex_);

    if (docId >= docClickCountList_->size())
    {
        return false;
    }

    count_t count = 0;
    docClickCountList_->getValue(docId, count);
    ++count;
    docClickCountList_->setValue(docId, count);
    updateDB(docId, count);

    return true;
}

bool CTRManager::getClickCountListByDocIdList(
        const std::vector<docid_t>& docIdList,
        std::vector<std::pair<size_t, count_t> >& posClickCountList)
{
    ScopedReadLock lock(mutex_);

    bool result = false;
    const size_t listSize = docIdList.size();
    posClickCountList.resize(listSize);
    for (size_t pos = 0; pos < listSize; ++pos)
    {
        count_t count = 0;
        if (docClickCountList_->getValue(docIdList[pos], count))
            result = true;
        posClickCountList[pos].first = pos;
        posClickCountList[pos].second = count;
    }

    return result;
}

bool CTRManager::getClickCountListByDocIdList(
        const std::vector<docid_t>& docIdList,
        std::vector<count_t>& clickCountList)
{
    ScopedReadLock lock(mutex_);

    clickCountList.resize(docIdList.size());

    bool result = false;
    for (size_t i = 0; i < docIdList.size(); i++)
    {
        if (docClickCountList_->getValue(docIdList[i], clickCountList[i]))
            result = true;
    }

    return result;
}

void CTRManager::loadCtrData(boost::shared_ptr<NumericPropertyTableBase>& numericPropertyTable)
{
    ScopedReadLock lock(mutex_);

    numericPropertyTable = static_pointer_cast<NumericPropertyTableBase>(docClickCountList_);
}

count_t CTRManager::getClickCountByDocId(docid_t docId)
{
    if (docId < docClickCountList_->size())
    {
        count_t count = 0;
        docClickCountList_->getValue(docId, count);
        return count;
    }

    return 0;
}

/// private

bool CTRManager::updateDB(docid_t docId, count_t clickCount)
{
    if (db_->update(docId, clickCount))
    {
        db_->flush();
        return true;
    }

    return false;
}

NS_FACETED_END
