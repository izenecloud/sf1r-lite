#include "property_diversity_reranker.h"

#include <mining-manager/group-label-logger/GroupLabelLogger.h>

#include <list>
#include <algorithm>
#include <glog/logging.h>

using sf1r::GroupLabelLogger;

NS_FACETED_BEGIN

PropertyDiversityReranker::PropertyDiversityReranker(
    const std::string& property,
    const GroupManager::PropValueMap& propValueMap,
    const std::string& boostingProperty
)
    :property_(property)
    ,propValueMap_(propValueMap)
    ,groupLabelLogger_(NULL)
    ,boostingProperty_(boostingProperty)
{
}

PropertyDiversityReranker::~PropertyDiversityReranker()
{
}

void PropertyDiversityReranker::simplererank(
    std::vector<unsigned int>& docIdList,
    std::vector<float>& rankScoreList
)
{
    typedef std::list<std::pair<unsigned int,float> > DocIdList;
    typedef std::map<PropValueTable::pvid_t, DocIdList> DocIdMap;

    GroupManager::PropValueMap::const_iterator pvIt = propValueMap_.find(property_);
    if (pvIt == propValueMap_.end())
    {
        LOG(ERROR) << "in GroupManager: group index file is not loaded for group property " << property_;
        return;
    }

    const PropValueTable& pvTable = pvIt->second;
    const std::vector<PropValueTable::pvid_t>& idTable = pvTable.propIdTable();

    std::size_t numDoc = docIdList.size();
    std::vector<unsigned int> newDocIdList;
    std::vector<float> newScoreList;	
    newDocIdList.reserve(numDoc);
    newScoreList.reserve(numDoc);

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
        DocIdMap::iterator mapIt = docIdMap.begin();
        while(mapIt != docIdMap.end())
        {
            if(mapIt->second.empty()) 
            {
                docIdMap.erase(mapIt++);
            }
            else
            {
                std::pair<unsigned int, float> element = mapIt->second.front();
                mapIt->second.pop_front();
                newDocIdList.push_back(element.first);
                newScoreList.push_back(element.second);
                ++mapIt;
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

void PropertyDiversityReranker::rerank(
    std::vector<unsigned int>& docIdList,
    std::vector<float>& rankScoreList,
    const std::string& query
)
{
    typedef std::list<std::pair<unsigned int,float> > DocIdList;
    typedef std::map<PropValueTable::pvid_t, DocIdList> DocIdMap;

    std::vector<std::string> propValueVec;
    std::vector<int> freqVec;
    if(groupLabelLogger_)
    {
        groupLabelLogger_->getFreqLabel(query, 1, propValueVec, freqVec);
    }

    if(!propValueVec.empty())
    {
        std::string& boostingCategoryLabel = propValueVec[0];
        //std::cout<<"boosting category "<<boostingCategoryLabel<<std::endl;		
        GroupManager::PropValueMap::const_iterator pvIt = propValueMap_.find(boostingProperty_);
        if (pvIt == propValueMap_.end())
        {
            LOG(ERROR) << "in GroupManager: group index file is not loaded for group property " << boostingProperty_;
            simplererank(docIdList, rankScoreList);
            return;
        }
        const PropValueTable& pvTable = pvIt->second;
        const std::vector<PropValueTable::pvid_t>& idTable = pvTable.propIdTable();
        const std::vector<PropValueTable::pvid_t>& parentTable = pvTable.parentIdTable();
        izenelib::util::UString labelUStr(boostingCategoryLabel, izenelib::util::UString::UTF_8);
        PropValueTable::pvid_t labelId = pvTable.propValueId(labelUStr);
        if (labelId == 0)
        {
            LOG(WARNING) << "in GroupManager: group index has not been built for Category value: " << boostingCategoryLabel;
            simplererank(docIdList, rankScoreList);			
            return;
        }

        std::size_t numDoc = docIdList.size();
		
        std::vector<unsigned int> boostingDocIdList;
        std::vector<float> boostingScoreList;	
        boostingDocIdList.reserve(numDoc);;
        boostingScoreList.reserve(numDoc);

        std::vector<unsigned int> leftDocIdList;
        std::vector<float> leftScoreList;	
        leftDocIdList.reserve(numDoc);;
        leftScoreList.reserve(numDoc);
		
        for(std::size_t i = 0; i < numDoc; ++i)
        {
            docid_t docId = docIdList[i];

            // this doc has not built group index data
            if (docId >= idTable.size())
                continue;

            PropValueTable::pvid_t pvId = idTable[docId];

            // check whether pvId is the child (or itself) of labelId
            bool isChild = false;
            while (pvId != 0)
            {
                if (pvId == labelId)
                {
                    isChild = true;
                    break;
                }
                pvId = parentTable[pvId];
            }

            if (isChild)
            {
                boostingDocIdList.push_back(docId);
                boostingScoreList.push_back(rankScoreList[i]);
            }
            else
            {
                leftDocIdList.push_back(docId);
                leftScoreList.push_back(rankScoreList[i]);
            }
        }
        if(!boostingDocIdList.empty())
        {
            simplererank(boostingDocIdList, boostingScoreList);
        }
        if(!leftDocIdList.empty())
        {
            simplererank(leftDocIdList, leftScoreList);
        }
        boostingDocIdList.resize(numDoc);
        boostingScoreList.resize(numDoc);
        std::copy_backward(leftDocIdList.begin(), leftDocIdList.end(), boostingDocIdList.end());
        std::copy_backward(leftScoreList.begin(), leftScoreList.end(), boostingScoreList.end());
        using std::swap;
        swap(docIdList, boostingDocIdList);
        swap(rankScoreList, boostingScoreList);
    }
    else
        simplererank(docIdList, rankScoreList);
}

NS_FACETED_END

