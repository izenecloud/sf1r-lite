#include "ProductScoreReader.h"
#include "ProductScoreTable.h"

using namespace sf1r;

ProductScoreReader::ProductScoreReader(
    const ProductScoreConfig& config,
    const ProductScoreTable& scoreTable)
    : ProductScorer(config)
    , productScoreTable_(scoreTable)
    , lock_(scoreTable.getMutex())
{
}

score_t ProductScoreReader::score(docid_t docId)
{
    return productScoreTable_.getScoreNoLock(docId);
}
