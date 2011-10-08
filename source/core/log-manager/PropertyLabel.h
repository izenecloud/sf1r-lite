#ifndef _PROPERTY_LABEL_H_
#define _PROPERTY_LABEL_H_

#include "DbRecordBase.h"
#include <boost/date_time/posix_time/posix_time.hpp>

namespace sf1r {

class PropertyLabel : public DbRecordBase {

public:

    enum Column { Collection, PropertyName, LabelName, HitDocsNum, TimeStamp, EoC };

    static const char* ColumnName[EoC];

    static const char* ColumnMeta[EoC];

    static const char* TableName;

    DEFINE_DB_RECORD_COMMON_ROUTINES(PropertyLabel)

    PropertyLabel() : DbRecordBase(),
        collectionPresent_(false),
        propertyNamePresent_(false),
        labelNamePresent_(false),
        hitDocsNumPresent_(false),
        timeStampPresent_(false){}

    ~PropertyLabel(){}

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

    inline const std::string & getPropertyName()
    {
        return propertyName_;
    }

    inline void setPropertyName( const std::string & propertyName ) {
        propertyName_ = propertyName;
        propertyNamePresent_ = true;
    }

    inline bool hasPropertyName() {
        return propertyNamePresent_;
    }

    inline const std::string & getLabelName()
    {
        return labelName_;
    }

    inline void setLabelName( const std::string & labelName ) {
        labelName_ = labelName;
        labelNamePresent_ = true;
    }

    inline bool hasLabelName() {
        return labelNamePresent_;
    }

    inline const size_t getHitDocsNum()
    {
        return hitDocsNum_;
    }

    inline void setHitDocsNum( const size_t hitDocsNum ) {
        hitDocsNum_ = hitDocsNum;
        hitDocsNumPresent_ = true;
    }

    inline bool hasHitDocsNum() {
        return hitDocsNumPresent_;
    }

    inline const boost::posix_time::ptime & getTimeStamp() {
        return timeStamp_;
    }

    inline void setTimeStamp( const boost::posix_time::ptime & timeStamp ) {
        timeStamp_ = timeStamp;
        timeStampPresent_ = true;
    }

    inline bool hasTimeStamp() {
        return timeStampPresent_;
    }

    void save( std::map<std::string, std::string> & rawdata );

    void load( const std::map<std::string, std::string> & rawdata );

private:

    std::string collection_;
    bool collectionPresent_;

    std::string propertyName_;
    bool propertyNamePresent_;

    std::string labelName_;
    bool labelNamePresent_;

    size_t hitDocsNum_;
    bool hitDocsNumPresent_;

    boost::posix_time::ptime timeStamp_;
    bool timeStampPresent_;
};

}
#endif
