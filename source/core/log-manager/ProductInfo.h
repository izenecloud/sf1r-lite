/**
 * \file ProductInfo.h
 * \brief 
 * \date Sep 9, 2011
 * \author Xin Liu
 */

#ifndef _PRODUCT_INFO_H_
#define _PRODUCT_INFO_H_

#include "DbRecordBase.h"
#include <boost/date_time/posix_time/posix_time.hpp>

namespace sf1r {

class ProductInfo : public DbRecordBase {

public:

    enum Column { Source, Collection, Num, Flag, TimeStamp, EoC };

    static const char* ColumnName[EoC];

    static const char* ColumnMeta[EoC];

    static const char* TableName;

    DEFINE_DB_RECORD_COMMON_ROUTINES(ProductInfo)

    ProductInfo() : DbRecordBase(),
        sourcePresent_(false),
        collectionPresent_(false),
        numPresent_(false),
        timeStampPresent_(false){}

    ~ProductInfo(){}

    inline const std::string & getSource()
    {
        return source_;
    }

    inline void setSource( const std::string & source ) {
        source_ = source;
        sourcePresent_ = true;
    }

    inline bool hasSource() {
        return sourcePresent_;
    }

    inline const std::string & getCollection()
    {
        return collection_;
    }

    inline void setCollection( const std::string & collection ) {
        collection_ = collection;
        collectionPresent_ = true;
    }

    inline bool hasCollection() {
        return collectionPresent_;
    }

    inline const uint32_t getNum()
    {
        return num_;
    }

    inline void setNum(const uint32_t num)
    {
        num_ = num;
        numPresent_ = true;
    }

    inline bool hasNum()
    {
        return numPresent_;
    }

    inline const std::string& getFlag()
    {
        return flag_;
    }

    inline void setFlag( const std::string& flag )
    {
        flag_ = flag;
        flagPresent_ = true;
    }

    inline bool hasFlag()
    {
        return flagPresent_;
    }

    inline void setTimeStamp( const boost::posix_time::ptime & timeStamp ) {
        timeStamp_ = timeStamp;
        timeStampPresent_ = true;
    }

    inline const boost::posix_time::ptime & getTimeStamp() {
        return timeStamp_;
    }

    inline bool hasTimeStamp() {
        return timeStampPresent_;
    }

    void save( std::map<std::string, std::string> & rawdata );

    void load( const std::map<std::string, std::string> & rawdata );

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
#endif /*_PRODUCT_INFO_H_ */
