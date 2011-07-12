#include "property_diversity_reranker.h"

#include <list>
#include <glog/logging.h>

NS_FACETED_BEGIN

PropertyDiversityReranker::PropertyDiversityReranker(
    const std::string& property,
    GroupManager* groupManager
)
    :property_(property)
    ,groupManager_(groupManager)
{
}

PropertyDiversityReranker::~PropertyDiversityReranker()
{
}

void PropertyDiversityReranker::rerank(
    std::vector<unsigned int>& docIdList,
    std::vector<float>& rankScoreList
)
{
    typedef std::list<std::pair<unsigned int,float> > DocIdList;
    typedef std::map<PropValueTable::pvid_t, DocIdList> DocIdMap;

    GroupManager::PropValueMap::const_iterator pvIt = groupManager_->propValueMap_.find(property_);
    if (pvIt == groupManager_->propValueMap_.end())
    {
        LOG(ERROR) << "in GroupManager: group index file is not loaded for group property " << property_;
        return;
    }

    const PropValueTable& pvTable = pvIt->second;
    const std::vector<PropValueTable::pvid_t>& idTable = pvTable.propIdTable();

    std::size_t numDoc = docIdList.size();
    std::vector<unsigned int> newDocIdList;
    std::vector<float> newScoreList;	
    newDocIdList.reserve(numDoc);;
    rankScoreList.reserve(numDoc);

    DocIdMap docIdMap;
    DocIdList missDocs;
    for (std::size_t i = 0; i < numDoc; ++i)
    {
        docid_t docId = docIdList[i];
	
        // this doc has not built group index data
        if (docId >= idTable.size())
            continue;
	
        PropValueTable::pvid_t pvId = idTable[docId];
        if (pvId != 0)
        {
            docIdMap[pvId].push_back(std::make_pair(docId, rankScoreList[i]));
        }
        else
        {
            missDocs.push_back(std::make_pair(docId, rankScoreList[i]));
        }
    }
    if(docIdMap.size() <= 1) return; // single property or empty
    do{
        for(DocIdMap::iterator mapIt = docIdMap.begin(); mapIt != docIdMap.end(); ++mapIt)
        {
            if(mapIt->second.empty()) 
            {
                docIdMap.erase(mapIt);
            }
            else
            {
                std::pair<unsigned int, float> element = mapIt->second.front();
                mapIt->second.pop_front();
                newDocIdList.push_back(element.first);
                newScoreList.push_back(element.second);
            }
        }
    }while(!docIdMap.empty());

    for(DocIdList::iterator missIt = missDocs.begin(); missIt != missDocs.end(); ++missIt)
    {
        newDocIdList.push_back(missIt->first);
        newScoreList.push_back(missIt->second);
    }

    using std::swap;
    swap(docIdList, newDocIdList);
    swap(rankScoreList, newScoreList);
}

NS_FACETED_END

