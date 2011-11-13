#ifndef _COLLECTION_INFO_H_
#define _COLLECTION_INFO_H_

#include "ColumnFamilyBase.h"

#include <boost/tuple/tuple.hpp>

namespace sf1r {

class CollectionInfo : public ColumnFamilyBase
{
public:
    typedef boost::tuple<uint32_t, std::string, std::string> SourceCountItemType;
    typedef std::map<time_t, SourceCountItemType> SourceCountType;

    static const std::string SuperColumnName[];

    CollectionInfo(const std::string& collection = "");

    ~CollectionInfo();

    virtual bool updateRow() const;

    virtual bool deleteRow();

    virtual bool getRow();

    virtual void resetKey(const std::string& newCollection = "");

    void insertSourceCount(const SourceCountItemType& sourceCountItem);

    void insertSourceCount(time_t timeStamp, const SourceCountItemType& sourceCountItem);

    DEFINE_COLUMN_FAMILY_COMMON_ROUTINES( CollectionInfo )

    inline const std::string& getCollection() const
    {
        return collection_;
    }

    inline void setCollection(const std::string& collection)
    {
        collection_ = collection;
        collectionPresent_ = true;
    }

    inline bool hasCollection() const
    {
        return collectionPresent_;
    }

    inline const SourceCountType& getSourceCount() const
    {
        return sourceCount_;
    }

    inline void setSourceCount(const SourceCountType& sourceCount)
    {
        sourceCount_ = sourceCount;
        sourceCountPresent_ = true;
    }

    inline bool hasSourceCount() const
    {
        return sourceCountPresent_;
    }

private:
    std::string collection_;
    bool collectionPresent_;

    SourceCountType sourceCount_;
    bool sourceCountPresent_;
};

}
#endif /*_COLLECTION_INFO_H_ */
