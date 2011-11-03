#ifndef _SYSTEM_EVENT_H_
#define _SYSTEM_EVENT_H_

#include "RDbRecordBase.h"
#include <boost/date_time/posix_time/posix_time.hpp>

namespace sf1r {

class SystemEvent : public RDbRecordBase {

public:

    enum Column { Level, Source, Content, TimeStamp, EoC };

    static const char* ColumnName[EoC];

    static const char* ColumnMeta[EoC];

    static const char* TableName;

    DEFINE_RDB_RECORD_COMMON_ROUTINES(SystemEvent)

    SystemEvent() : RDbRecordBase(),
        levelPresent_(false),
        sourcePresent_(false),
        contentPresent_(false),
        timeStampPresent_(false){}

    ~SystemEvent(){}

    inline const std::string & getLevel() const
    {
        return level_;
    }

    inline void setLevel( const std::string & level )
    {
        level_ = level;
        levelPresent_ = true;
    }

    inline bool hasLevel() const
    {
        return levelPresent_;
    }

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

    inline const std::string & getContent() const
    {
        return content_;
    }

    inline void setContent( const std::string & content )
    {
        content_ = content;
        contentPresent_ = true;
    }

    inline bool hasContent() const
    {
        return contentPresent_;
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

private:

    std::string level_;
    bool levelPresent_;

    std::string source_;
    bool sourcePresent_;

    std::string content_;
    bool contentPresent_;

    boost::posix_time::ptime timeStamp_;
    bool timeStampPresent_;
};

}
#endif
