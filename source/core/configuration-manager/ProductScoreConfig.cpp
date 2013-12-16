#include "ProductScoreConfig.h"
#include "ProductRankingConfig.h"
#include <sstream>

using namespace sf1r;

ProductScoreConfig::ProductScoreConfig()
    : type(PRODUCT_SCORE_NUM)
    , weight(0)
    , minLimit(0)
    , maxLimit(0)
    , zoomin(1)
    , isDebug(false)
{}

void ProductScoreConfig::limitScore(score_t& score) const
{
    if (minLimit != 0 && score < minLimit)
    {
        score = minLimit;
    }
    else if (maxLimit != 0 && score > maxLimit)
    {
        score = maxLimit;
    }
}

bool ProductScoreConfig::isValidScore(score_t score) const
{
    if (minLimit != 0 && score < minLimit)
        return false;

    if (maxLimit != 0 && score > maxLimit)
        return false;

    return true;
}

std::string ProductScoreConfig::toStr() const
{
    std::ostringstream oss;

    const std::string& typeName = ProductRankingConfig::kScoreTypeName[type];
    oss << "score type: " << typeName << ", ";

    if (!propName.empty())
    {
        oss << "property: " << propName << ", ";
    }

    oss << "weight: " << weight
        << ", min: " << minLimit
        << ", max: " << maxLimit
        << ", zoomin: " << zoomin
        << std::endl;

    for (std::size_t i = 0; i < factors.size(); ++i)
    {
        oss << "factor: " << i << ", "
            << factors[i].toStr();
    }

    return oss.str();
}
