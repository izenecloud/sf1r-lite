#ifndef _SOURCE_COUNT_H_
#define _SOURCE_COUNT_H_

#include "ColumnFamilyBase.h"

namespace sf1r {

class SourceCount : public ColumnFamilyBase
{
public:
    typedef std::map<std::string, int64_t> SourceCountType;

    explicit SourceCount(const std::string& collection = "");

    ~SourceCount();

    virtual const std::string& getKey() const;

    virtual bool updateRow() const;

    virtual void insertCounter(const std::string& name, int64_t value);

    virtual void resetKey(const std::string& newCollection = "");

    virtual void clear();

    DEFINE_COLUMN_FAMILY_COMMON_ROUTINES( SourceCount )

    inline const std::string& getCollection() const
    {
        return collection_;
    }

    inline void setCollection(const std::string& collection)
    {
        collection_ = collection;
    }

    inline bool hasCollection() const
    {
        return !collection_.empty();
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

    SourceCountType sourceCount_;
    bool sourceCountPresent_;
};

}
#endif /*_SOURCE_COUNT_H_ */
