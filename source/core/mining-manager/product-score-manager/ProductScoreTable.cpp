#include "ProductScoreTable.h"
#include "../util/fcontainer_febird.h"

using namespace sf1r;

namespace
{
const char* FILE_NAME_SUFFIX = ".bin";
}

ProductScoreTable::ProductScoreTable(
    const std::string& dirPath,
    const std::string& scoreTypeName)
    : dirPath_(dirPath)
    , scoreTypeName_(scoreTypeName)
{
}

bool ProductScoreTable::open()
{
    ScopedWriteLock lock(mutex_);

    return load_container_febird(dirPath_,
                                 scoreTypeName_ + FILE_NAME_SUFFIX,
                                 scores_);
}

bool ProductScoreTable::flush()
{
    ScopedWriteLock lock(mutex_);

    return save_container_febird(dirPath_,
                                 scoreTypeName_ + FILE_NAME_SUFFIX,
                                 scores_);
}

void ProductScoreTable::resize(std::size_t num)
{
    ScopedWriteLock lock(mutex_);

    scores_.resize(num);
}

void ProductScoreTable::setScore(docid_t docId, score_t score)
{
    ScopedWriteLock lock(mutex_);

    scores_[docId] = score;
}

score_t ProductScoreTable::getScore(docid_t docId) const
{
    if (docId < scores_.size())
        return scores_[docId];

    return 0;
}
