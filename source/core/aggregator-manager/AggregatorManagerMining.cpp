#include "AggregatorManager.h"

namespace sf1r
{

void AggregatorManager::aggregateSimilarDocIdList(SimilarDocIdListType& result, const std::vector<std::pair<workerid_t, SimilarDocIdListType> >& resultList)
{
    size_t subResultNum = resultList.size();

    if (subResultNum <= 0)
        return;

    size_t totalDocNum = 0;
    for (size_t i = 0; i < subResultNum; i++)
    {
        totalDocNum += resultList[i].second.size();
    }

    float maxScore;
    size_t maxi;
    size_t* iter = new size_t[subResultNum];
    memset(iter, 0, sizeof(size_t)*subResultNum);

    result.reserve(totalDocNum);
    while(1)
    {
        // find max from head elements of lists
        maxScore = -1;
        maxi = size_t(-1);
        for (size_t i = 0; i < subResultNum; i++)
        {
            const SimilarDocIdListType& simDocList = resultList[i].second;
            if (iter[i] >= simDocList.size())
                continue;

            if (simDocList[iter[i]].second > maxScore)
            {
                maxScore = simDocList[iter[i]].second;
                maxi = i;
            }
        }

        //cout << "maxi: "<< maxi<<", iter[maxi]: " << iter[maxi]<<endl;
        if (maxi == size_t(-1))
        {
            break;
        }

        const SimilarDocIdListType& simDocList = resultList[maxi].second;
        result.push_back(simDocList[iter[maxi]]);
        iter[maxi]++;
    }

    delete[] iter;
}

}
