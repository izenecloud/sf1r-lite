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
    db_ = new DBType(filePath_);

    if (!db_ || !db_->open())
    {
        return false;
    }

    if (docClickCountList_.size() < docNum_+1)
        docClickCountList_.resize(docNum_+1, 0);

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

bool CTRManager::update(uint32_t docId)
{
    if (docId >= docClickCountList_.size())
    {
        return false;
    }

    docClickCountList_[docId] ++;

    updateDB(docId, docClickCountList_[docId]);

    return true;
}

/// private

bool CTRManager::updateDB(uint32_t docId, count_t clickCount)
{
    if (db_->update(docId, clickCount))
    {
        db_->flush();
    }

    return false;
}

NS_FACETED_END


