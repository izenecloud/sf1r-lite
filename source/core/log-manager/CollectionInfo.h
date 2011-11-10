#ifndef _COLLECTION_INFO_H_
#define _COLLECTION_INFO_H_

#include "CassandraColumnFamily.h"

#include <boost/tuple/tuple.hpp>

namespace sf1r {

class CollectionInfo : public CassandraColumnFamily
{
public:
    CollectionInfo(const std::string& collection = "");

    ~CollectionInfo();

    bool updateRow() const;

    bool deleteRow();

    bool getRow();

    bool save();

    void reset(const std::string& newCollection = "");

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

    inline const std::string& getSource() const
    {
        return source_;
    }

    inline void setSource(const std::string& source)
    {
        source_ = source;
        sourcePresent_ = true;
    }

    inline bool hasSource() const
    {
        return sourcePresent_;
    }

    inline const uint32_t getCount() const
    {
        return count_;
    }

    inline void setCount(const uint32_t count)
    {
        count_ = count;
        countPresent_ = true;
    }

    inline bool hasCount() const
    {
        return countPresent_;
    }

    inline const std::string& getFlag() const
    {
        return flag_;
    }

    inline void setFlag(const std::string& flag)
    {
        flag_ = flag;
        flagPresent_ = true;
    }

    inline bool hasFlag() const
    {
        return flagPresent_;
    }

    inline const boost::posix_time::ptime& getTimeStamp() const
    {
        return timeStamp_;
    }

    inline void setTimeStamp(const boost::posix_time::ptime& timeStamp)
    {
        timeStamp_ = timeStamp;
        timeStampPresent_ = true;
    }

    inline bool hasTimeStamp() const
    {
        return timeStampPresent_;
    }

private:
    std::string collection_;
    bool collectionPresent_;

    std::string source_;
    bool sourcePresent_;

    uint32_t count_;
    bool countPresent_;

    std::string flag_;
    bool flagPresent_;

    boost::posix_time::ptime timeStamp_;
    bool timeStampPresent_;
};

}
#endif /*_COLLECTION_INFO_H_ */
