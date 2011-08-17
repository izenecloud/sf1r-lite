#include "ctr_manager.h"

NS_FACETED_BEGIN


CTRManager::CTRManager(const std::string& dirPath, size_t docNum)
: docNum_(docNum)
{
    filePath_ = dirPath + "/ctr.db";

    // docid start from 1
    docClickCountList_.resize(docNum+1, 0);
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
    boost::lock_guard<boost::shared_mutex> lg(mutex_);

    db_ = new DBType(filePath_);

    if (!db_ || !db_->open())
    {
        return false;
    }

    size_t nums = db_->num_items();
    if (nums > 0)
    {
        uint32_t k;
        count_t v;

        DBType::SDBCursor locn = db_->get_first_locn();
        while ( db_->get(locn, k, v) )
        {
            if (k < docClickCountList_.size())
            {
                docClickCountList_[k] = v;
            }

            db_->seq(locn);
        }
    }

    return true;
}

/// TODO, remove lock to reduce overhead.
/// use atomic operation, or it can only update disk(db) data here,
/// memory data can be loaded from disk wholly periodically.
bool CTRManager::update(uint32_t docId)
{
    boost::lock_guard<boost::shared_mutex> lg(mutex_);

    if (docId >= docClickCountList_.size())
    {
        return false;
    }

    docClickCountList_[docId] ++;

    updateDB(docId, docClickCountList_[docId]);

    return true;
}

void CTRManager::getClickCountListByDocIdList(
        const std::vector<unsigned int>& docIdList,
        std::vector<std::pair<size_t, count_t> >& posClickCountList)
{
    boost::lock_guard<boost::shared_mutex> lg(mutex_);

    if (docClickCountList_.size() <= 0)
        return;

    for (size_t pos = 0; pos < docIdList.size(); pos++)
    {
        const unsigned int& docId = docIdList[pos];
        if (docId < docClickCountList_.size())
        {
            if (docClickCountList_[docId] > 0)
            {
                posClickCountList.push_back(std::make_pair(pos, docClickCountList_[docId]));
            }
        }
    }
}

void CTRManager::getClickCountListByDocIdList(
        const std::vector<unsigned int>& docIdList,
        std::vector<count_t>& clickCountList)
{
    boost::lock_guard<boost::shared_mutex> lg(mutex_);

    clickCountList.resize(docIdList.size(), 0);

    for (size_t i = 0; i < docIdList.size(); i++)
    {
        const unsigned int& docId = docIdList[i];
        if (docId < docClickCountList_.size())
        {
            clickCountList[i] = docClickCountList_[docId];
        }
    }
}

count_t CTRManager::getClickCountByDocId(uint32_t docId)
{
    boost::lock_guard<boost::shared_mutex> lg(mutex_);

    if (docId < docClickCountList_.size())
    {
        return docClickCountList_[docId];
    }

    return 0;
}

bool CTRManager::getClickCountByDocId(uint32_t docId, count_t& clickCount)
{
    boost::lock_guard<boost::shared_mutex> lg(mutex_);

    if (docId < docClickCountList_.size())
    {
        clickCount = docClickCountList_[docId];
        return true;
    }

    clickCount = 0;
    return false;
}


/// private

bool CTRManager::updateDB(uint32_t docId, count_t clickCount)
{
    if (db_->update(docId, clickCount))
    {
        db_->flush();
        return true;
    }

    return false;
}

NS_FACETED_END


