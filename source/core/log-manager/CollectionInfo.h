#ifndef _COLLECTION_INFO_H_
#define _COLLECTION_INFO_H_

#include "CassandraColumnFamily.h"

#include <boost/date_time/posix_time/posix_time.h>

namespace sf1r {

class CollectionInfo : public CassandraColumnFamily
{
public:
    CollectionInfo() : CassandraColumnFamily() {}

    ~CollectionInfo() {}

    void save();

    DECLARE_COLUMN_FAMILY_COMMON_ROUTINES

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

    inline const uint32_t getNum() const
    {
        return num_;
    }

    inline void setNum(const uint32_t num)
    {
        num_ = num;
        numPresent_ = true;
    }

    inline bool hasNum() const
    {
        return numPresent_;
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

    std::string source_;
    bool sourcePresent_;

    std::string collection_;
    bool collectionPresent_;

    uint32_t num_;
    bool numPresent_;

    std::string flag_;
    bool flagPresent_;

    boost::posix_time::ptime timeStamp_;
    bool timeStampPresent_;
};

}
#endif /*_COLLECTION_INFO_H_ */
