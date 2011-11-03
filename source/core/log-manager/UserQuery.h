#ifndef _USER_QUERY_H_
#define _USER_QUERY_H_

#include "RDbRecordBase.h"
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/algorithm/string/replace.hpp>

namespace sf1r {

class UserQuery : public RDbRecordBase {

public:

    enum Column { Query, Collection, HitDocsNum, PageStart, PageCount, SessionId, Duration, TimeStamp, EoC };

    static const char* ColumnName[EoC];

    static const char* ColumnMeta[EoC];

    static const char* TableName;

    DEFINE_RDB_RECORD_COMMON_ROUTINES(UserQuery)

    UserQuery() : RDbRecordBase(),
        queryPresent_(false),
        collectionPresent_(false),
        hitDocsNumPresent_(false),
        pageStartPresent_(false),
        pageCountPresent_(false),
        sessionIdPresent_(false),
        durationPresent_(false),
        timeStampPresent_(false){}

    ~UserQuery(){}

    inline const std::string & getQuery() const
    {
        return query_;
    }

    inline void setQuery( const std::string & query )
    {
        query_ = boost::replace_all_copy(query, "\"", "\"\"");
        queryPresent_ = true;
    }

    inline bool hasQuery() const
    {
        return queryPresent_;
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

    inline const uint32_t getPageStart() const
    {
        return pageStart_;
    }

    inline void setPageStart( const uint32_t pageStart )
    {
        pageStart_ = pageStart;
        pageStartPresent_ = true;
    }

    inline bool hasPageStart() const
    {
        return pageStartPresent_;
    }

    inline const uint32_t getPageCount() const
    {
        return pageCount_;
    }

    inline void setPageCount( const uint32_t pageCount )
    {
        pageCount_ = pageCount;
        pageCountPresent_ = true;
    }

    inline bool hasPageCount() const
    {
        return pageCountPresent_;
    }

    inline const std::string & getSessionId() const
    {
        return sessionId_;
    }

    inline void setSessionId( const std::string & sessionId )
    {
        sessionId_ = sessionId;
        sessionIdPresent_ = true;
    }

    inline bool hasSessionId() const
    {
        return sessionIdPresent_;
    }

    inline const boost::posix_time::time_duration & getDuration() const
    {
        return duration_;
    }

    inline void setDuration( const boost::posix_time::time_duration & duration )
    {
        duration_ = duration;
        durationPresent_ = true;
    }

    inline bool hasDuration() const
    {
        return durationPresent_;
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

    std::string query_;
    bool queryPresent_;

    std::string collection_;
    bool collectionPresent_;

    size_t hitDocsNum_;
    bool hitDocsNumPresent_;

    size_t pageStart_;
    bool pageStartPresent_;

    size_t pageCount_;
    bool pageCountPresent_;

    std::string sessionId_;
    bool sessionIdPresent_;

    boost::posix_time::time_duration duration_;
    bool durationPresent_;

    boost::posix_time::ptime timeStamp_;
    bool timeStampPresent_;
};

}
#endif
