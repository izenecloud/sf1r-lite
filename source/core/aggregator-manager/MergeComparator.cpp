#include "MergeComparator.h"

namespace sf1r{



DocumentComparator::DocumentComparator(const KeywordSearchResult& distSearchResult)
{
    const DistKeywordSearchInfo& distSearchInfo = distSearchResult.distSearchInfo_;
    const std::vector<std::pair<std::string , bool> >& sortPropertyList = distSearchInfo.sortPropertyList_;
    if (sortPropertyList.size() <= 0)
        const_cast<std::vector<std::pair<std::string , bool> >&>(sortPropertyList).push_back(std::make_pair("RANK", false));
    std::vector<std::pair<std::string , bool> >::const_iterator iter;
    for (iter = sortPropertyList.begin(); iter != sortPropertyList.end(); ++iter)
    {
        std::string property = iter->first;
        SortPropertyData* pPropertyComparator = new SortPropertyData(iter->first, iter->second);

        void* dataList = NULL;
        if (property == "RANK")
        {
            dataList = (void*)(distSearchResult.topKRankScoreList_.data());
            pPropertyComparator->setDataType(SortPropertyData::DATA_TYPE_FLOAT);
        }
        else if (property == "CUSTOM_RANK")
        {
            dataList = (void*)(distSearchResult.topKCustomRankScoreList_.data());
            pPropertyComparator->setDataType(SortPropertyData::DATA_TYPE_FLOAT);
        }
        else
        {
            while (1)
            {
                bool found = false;
                std::vector<std::pair<std::string, std::vector<int32_t> > >::const_iterator it;
                for (it = distSearchInfo.sortPropertyInt32DataList_.begin(); it != distSearchInfo.sortPropertyInt32DataList_.end(); it++)
                {
                    if (it->first == property)
                    {
                        dataList = (void*)(it->second.data());
                        pPropertyComparator->setDataType(SortPropertyData::DATA_TYPE_INT32);
                        found = true;
                    }
                }
                if (found) break;

                std::vector<std::pair<std::string, std::vector<int64_t> > >::const_iterator itu;
                for (itu = distSearchInfo.sortPropertyInt64DataList_.begin(); itu != distSearchInfo.sortPropertyInt64DataList_.end(); itu++)
                {
                    if (itu->first == property)
                    {
                        dataList = (void*)(itu->second.data());
                        pPropertyComparator->setDataType(SortPropertyData::DATA_TYPE_INT64);
                        found = true;
                    }
                }
                if (found) break;

                std::vector<std::pair<std::string, std::vector<float> > >::const_iterator itf;
                for (itf = distSearchInfo.sortPropertyFloatDataList_.begin(); itf != distSearchInfo.sortPropertyFloatDataList_.end(); itf++)
                {
                    if (itf->first == property)
                    {
                        dataList = (void*)(itf->second.data());
                        pPropertyComparator->setDataType(SortPropertyData::DATA_TYPE_FLOAT);
                        found = true;
                    }
                }

                break;
            }
        }

        if (dataList !=  NULL)
        {
            pPropertyComparator->setDataList(dataList);

            sortProperties_.push_back(pPropertyComparator);
        }
        else
        {
            delete pPropertyComparator;
            std::cerr << "Merging, no data for sort property " << property <<endl;
        }
    }
}

DocumentComparator::~DocumentComparator()
{
    for (size_t i = 0; i < sortProperties_.size(); i++)
    {
        delete sortProperties_[i];
    }
}

bool greaterThan(DocumentComparator* comp1, size_t idx1, DocumentComparator* comp2, size_t idx2)
{
    for (size_t i = 0; i < comp1->sortProperties_.size(); i++)
    {
        if (i >= comp2->sortProperties_.size())
            break;

        SortPropertyData* pSortProperty1 = comp1->sortProperties_[i];
        SortPropertyData* pSortProperty2 = comp2->sortProperties_[i];

        SortPropertyData::DataType dataType1 = pSortProperty1->getDataType();
        SortPropertyData::DataType dataType2 = pSortProperty2->getDataType();
        if (dataType1 != dataType2)
            continue;

        void* dataList1 = pSortProperty1->getDataList();
        void* dataList2 = pSortProperty2->getDataList();

        if (dataType1 == SortPropertyData::DATA_TYPE_INT32)
        {
            int32_t v1 = ((int32_t*)dataList1)[idx1];
            int32_t v2 = ((int32_t*)dataList2)[idx2];
            if (v1 == v2) continue;
            if (v1 > v2) return true;
            if (v1 < v2) return false;
        }
        else if (dataType1 == SortPropertyData::DATA_TYPE_INT64)
        {
            int64_t v1 = ((int64_t*)dataList1)[idx1];
            int64_t v2 = ((int64_t*)dataList2)[idx2];
            if (v1 == v2) continue;
            if (v1 > v2) return true;
            if (v1 < v2) return false;
        }
        else if (dataType1 == SortPropertyData::DATA_TYPE_FLOAT)
        {
            float v1 = ((float*)dataList1)[idx1];
            float v2 = ((float*)dataList2)[idx2];
            if (v1 == v2) continue;
            if (v1 > v2) return true;
            if (v1 < v2) return false;
        }
    }

    return false;
}


}
