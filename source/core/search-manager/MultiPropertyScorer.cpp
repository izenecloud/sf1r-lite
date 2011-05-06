#include "MultiPropertyScorer.h"
#include <util/profiler/ProfilerGroup.h>

using namespace std;
using namespace sf1r;

double MultiPropertyScorer::score(
    const std::vector<RankQueryProperty>& rankQueryProperties,
    const std::vector<boost::shared_ptr<PropertyRanker> >& propertyRankers
)
{
    CREATE_PROFILER ( compute_score, "SearchManager", "doSearch_: compute score in multipropertyscorer");
    CREATE_PROFILER ( get_doc_item, "SearchManager", "doSearch_: get doc_item");

    DocumentIterator* pEntry = 0;
    double score = 0.0F;
    size_t numProperties = rankQueryProperties.size();
    for (size_t i = 0; i < numProperties; ++i)
    {
        pEntry = docIteratorList_[i];
        if (pEntry && pEntry->isCurrent())
        {
            double weight = propertyWeightList_[i];
            if (weight != 0.0F)
            {
                rankDocumentProperty_.reset();
                rankDocumentProperty_.resize(rankQueryProperties[i].size());
                pEntry->doc_item(rankDocumentProperty_);

                //START_PROFILER ( compute_score )
                score += weight * propertyRankers[i]->getScore(
                    rankQueryProperties[i],
                    rankDocumentProperty_
                );
                //STOP_PROFILER ( compute_score )

                //pEntry->queryBoosting(score, weight); ///xxx
            }
        }
    }

    if (! (score < (numeric_limits<float>::max) ()))
    {
        score = (numeric_limits<float>::max) ();
    }

    return score;
}

void MultiPropertyScorer::print(int level)
{
    cout << std::string(level*4, ' ') << "|--[ "<< "MultiPropertyScorer current: " << current_<<" "<< currDoc_ << " ]"<< endl;

    DocumentIterator* pEntry;
    for (size_t i = 0; i < docIteratorList_.size(); ++i)
    {
        pEntry = docIteratorList_[i];
        if (pEntry/* && pEntry->isCurrent()*/)
        {
            pEntry->print(level+1);
        }
    }
}
