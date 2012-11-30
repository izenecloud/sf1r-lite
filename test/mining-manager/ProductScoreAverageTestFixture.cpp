#include "ProductScoreAverageTestFixture.h"
#include "ProductScorerStub.h"
#include "../recommend-manager/test_util.h"

using namespace sf1r;

ProductScoreAverageTestFixture::ProductScoreAverageTestFixture()
{
    averageScoreConfig_.weight = 1;
    averageScorer_.reset(new ProductScoreAverage(averageScoreConfig_));
}

void ProductScoreAverageTestFixture::addScorers(
    const std::string& scoreList,
    const std::string& weightList)
{
    typedef std::vector<score_t> ScoreArray;
    ScoreArray stubScores;
    ScoreArray stubWeights;

    split_str_to_items(scoreList, stubScores);
    split_str_to_items(weightList, stubWeights);

    std::size_t num = stubScores.size();
    for (std::size_t i = 0; i < num; ++i)
    {
        ProductScoreConfig stubConfig;
        stubConfig.weight = stubWeights[i];

        ProductScorerStub* stub = new ProductScorerStub(stubConfig,
                                                        stubScores[i]);
        averageScorer_->addScorer(stub);
    }
}
