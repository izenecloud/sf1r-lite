#ifndef _PROPERTY_LABEL_H_
#define _PROPERTY_LABEL_H_

#include "RDbRecordBase.h"

namespace sf1r {

class PropertyLabel : public RDbRecordBase {

public:

    enum Column { Collection, LabelName, HitDocsNum, EoC };

    static const char* ColumnName[EoC];

    static const char* ColumnMeta[EoC];

    static const char* TableName;

    DEFINE_RDB_RECORD_COMMON_ROUTINES(PropertyLabel)

    PropertyLabel() : RDbRecordBase(),
        collectionPresent_(false),
        labelNamePresent_(false),
        hitDocsNumPresent_(false){}

    ~PropertyLabel(){}

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

    inline const std::string & getLabelName() const
    {
        return labelName_;
    }

    inline void setLabelName( const std::string & labelName )
    {
        labelName_ = labelName;
        labelNamePresent_ = true;
    }

    inline bool hasLabelName() const
    {
        return labelNamePresent_;
    }

    inline const size_t getHitDocsNum() const
    {
        return hitDocsNum_;
    }

    inline void setHitDocsNum( const size_t hitDocsNum )
    {
        hitDocsNum_ = hitDocsNum;
        hitDocsNumPresent_ = true;
    }

    inline bool hasHitDocsNum() const
    {
        return hitDocsNumPresent_;
    }

    void save( std::map<std::string, std::string> & rawdata );

    void load( const std::map<std::string, std::string> & rawdata );

    void save_to_logserver();
    static void get_from_logserver(const std::string& collection,
        std::list<std::map<std::string, std::string> >& res);
    static void del_from_logserver(const std::string& collection);
private:

    static const std::string service_;

    std::string collection_;
    bool collectionPresent_;

    std::string labelName_;
    bool labelNamePresent_;

    size_t hitDocsNum_;
    bool hitDocsNumPresent_;
};

}
#endif
