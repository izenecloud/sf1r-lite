#include "NumericPropertyScorerTestFixture.h"
#include "../recommend-manager/test_util.h"
#include <common/NumericPropertyTable.h>
#include <boost/test/unit_test.hpp>

using namespace sf1r;

namespace
{

const float FLOAT_TOLERANCE = 1e-5;

} // namespace

NumericPropertyScorerTestFixture::NumericPropertyScorerTestFixture()
    : numericTable_(new NumericPropertyTable<int>(INT32_PROPERTY_TYPE))
{
    numericScoreConfig_.weight = 1;
}

void NumericPropertyScorerTestFixture::setNumericValue(const std::string& numericList)
{
    typedef std::vector<int> NumericArray;
    NumericArray array;

    split_str_to_items(numericList, array);

    docid_t docId = 0;
    for (NumericArray::const_iterator it = array.begin();
         it != array.end(); ++it)
    {
        ++docId;
        numericTable_->setInt32Value(docId, *it);
    }
}

void NumericPropertyScorerTestFixture::setNumericValue(docid_t docId, int numericValue)
{
    numericTable_->setInt32Value(docId, numericValue);
}

void NumericPropertyScorerTestFixture::createScorer()
{
    numericScorer_.reset(new NumericPropertyScorer(numericScoreConfig_,
                                                   numericTable_));
}

void NumericPropertyScorerTestFixture::checkScore(docid_t docId, score_t gold)
{
    score_t result = numericScorer_->score(docId);

    BOOST_CHECK_CLOSE(result, gold, FLOAT_TOLERANCE);
}
