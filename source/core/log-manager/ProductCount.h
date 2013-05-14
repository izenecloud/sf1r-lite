/**
 * \file ProductCount.h
 * \brief
 * \date Sep 9, 2011
 * \author Xin Liu
 */

#ifndef _PRODUCT_COUNT_H_
#define _PRODUCT_COUNT_H_

#include "RDbRecordBase.h"
#include <boost/date_time/posix_time/posix_time.hpp>

namespace sf1r {

class ProductCount : public RDbRecordBase {

public:

    enum Column { Source, Collection, Num, Flag, TimeStamp, EoC };

    static const char* ColumnName[EoC];

    static const char* ColumnMeta[EoC];

    static const char* TableName;

    DEFINE_RDB_RECORD_COMMON_ROUTINES(ProductCount)

    ProductCount() : RDbRecordBase(),
        sourcePresent_(false),
        collectionPresent_(false),
        numPresent_(false),
        timeStampPresent_(false){}

    ~ProductCount(){}

    inline const std::string & getSource() const
    {
        return source_;
    }

    inline void setSource( const std::string & source )
    {
        source_ = source;
        sourcePresent_ = true;
    }

    inline bool hasSource() const
    {
        return sourcePresent_;
    }

    inline const std::string & getCollection() const
    {
        return collection_;
    }

    inline void setCollection( const std::string & collection )
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

    inline void setFlag( const std::string& flag )
    {
        flag_ = flag;
        flagPresent_ = true;
    }

    inline bool hasFlag() const
    {
        return flagPresent_;
    }

    inline const boost::posix_time::ptime & getTimeStamp() const
    {
        return timeStamp_;
    }

    inline void setTimeStamp( const boost::posix_time::ptime & timeStamp )
    {
        timeStamp_ = timeStamp;
        timeStampPresent_ = true;
    }

    inline bool hasTimeStamp() const
    {
        return timeStampPresent_;
    }

    void save( std::map<std::string, std::string> & rawdata );

    void load( const std::map<std::string, std::string> & rawdata );

    void save_to_logserver();

private:

    static const std::string service_;

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
#endif /*_PRODUCT_COUNT_H_ */
