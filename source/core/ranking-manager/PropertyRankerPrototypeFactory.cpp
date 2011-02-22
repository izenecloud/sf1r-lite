/**
 * @file core/ranking-manager/PropertyRankerPrototypeFactory.cpp
 * @author Ian Yang
 * @date Created <2009-09-24 17:04:44>
 * @date Updated <2009-12-22 18:27:28>
 */
#include "PropertyRankerPrototypeFactory.h"

#include "NullRanker.h"
#include "BM25Ranker.h"
#include "LanguageRanker.h"
#include "PlmLanguageRanker.h"
#include "ClosestPositionTermProximityMeasure.h"

#include <cstring>

namespace sf1r {

PropertyRankerPrototypeFactory::PropertyRankerPrototypeFactory()
{
    std::memset(rankers_, 0, sizeof rankers_);
}

PropertyRankerPrototypeFactory::~PropertyRankerPrototypeFactory()
{
    for (unsigned i = 0; i <= RankingType::NotUseTextRanker; ++i)
    {
        if (i != RankingType::DefaultTextRanker)
        {
            delete rankers_[i];
        }
    }
}

void PropertyRankerPrototypeFactory::init(
    const RankingConfigUnit& config
)
{
    delete rankers_[BM25];
    rankers_[BM25] = new BM25Ranker;

    delete rankers_[KL];
    rankers_[KL] = new LanguageRanker;

    delete rankers_[PLM];
    const TermProximityMeasure* termProximityMeasure =
        new AveClosestPositionTermProximityMeasure;
    rankers_[PLM] = new PlmLanguageRanker(termProximityMeasure);

    delete rankers_[NotUseTextRanker];
    rankers_[NotUseTextRanker] = new NullRanker;

    rankers_[DefaultTextRanker] = rankers_[config.textRankingModel_];
    if (DefaultTextRanker == config.textRankingModel_)
    {
        rankers_[DefaultTextRanker] = rankers_[NotUseTextRanker];
    }
}

const boost::shared_ptr<PropertyRanker>
PropertyRankerPrototypeFactory::createNullRanker() const
{
    static NullRanker ranker;
    boost::shared_ptr<PropertyRanker> ptr(
        &ranker,
        &PropertyRankerPrototypeFactory::deleteNullRanker_
    );
    return ptr;
}

} // namespace sf1r
