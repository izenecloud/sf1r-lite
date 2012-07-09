/**
 * @file MergeComparator.h
 * @author Zhongxia Li
 * @date Aug 22, 2011
 * @brief
 */
#ifndef MERGE_COMPARATOR_H_
#define MERGE_COMPARATOR_H_

#include <common/ResultType.h>


namespace sf1r
{

class SortPropertyData
{
public:
    enum DataType
    {
        DATA_TYPE_NONE,
        DATA_TYPE_INT32,
        DATA_TYPE_INT64,
        DATA_TYPE_FLOAT
    };

public:
    SortPropertyData(const std::string& property, bool reverse)
        : property_(property), reverse_(reverse), dataType_(DATA_TYPE_NONE), dataList_(NULL)
    {
    }

    std::string& getProperty()
    {
        return property_;
    }

    void setDataType(DataType dataType)
    {
        dataType_= dataType;
    }

    DataType getDataType()
    {
        return dataType_;
    }

    void setDataList(void* dataList)
    {
        dataList_ = dataList;
    }

    void* getDataList()
    {
        return dataList_;
    }

    bool isReverse()
    {
        return reverse_;
    }

private:
    std::string property_;
    bool reverse_;
    DataType dataType_;
    void* dataList_;
};

class DocumentComparator
{
public:
    DocumentComparator(const DistKeywordSearchResult& distSearchResult);

    ~DocumentComparator();

private:
    std::vector<SortPropertyData*> sortProperties_;

    friend bool greaterThan(DocumentComparator* comp1, size_t idx1, DocumentComparator* comp2, size_t idx2);
};

bool greaterThan(DocumentComparator* comp1, size_t idx1, DocumentComparator* comp2, size_t idx2);

}

#endif /* MERGE_COMPARATOR_H_ */
