#include <log-manager/LogManager.h>
#include <string>
#include <vector>
#include <boost/shared_ptr.hpp>
#include <boost/noncopyable.hpp>

#include <boost/function.hpp>
#include <common/type_defs.h>
using namespace sf1r;
class LogManagerTestDef {

//public:
//
//    LogManagerTestDef(LogManager* logManager):logManager_(logManager), list_(0), count_(0)
//    {
//        init_();
//    }
//
//
//public:
//
//    void insertQuery( const izenelib::util::UString& ustr, const boost::posix_time::ptime& time)
//    {
//        QueryLogRecord queryLog ;
//        queryLog.commonPart_.isQueryLog_ = true;
//        queryLog.commonPart_.sessionID_ = "session";
//        queryLog.commonPart_.query_= ustr;
//        queryLog.commonPart_.timeStamp_= time;
//        queryLog.pageStart_ = 0;
//        queryLog.pageCount_ = 10;
//        queryLog.hitDocsNum_ = 10;
//        logManager_->writeQueryLogByDate("", queryLog);
//        count_++;
//    }
//
//    bool isSucc()
//    {
//        logManager_->runEvents();
//        return list_.size() == count_;
//    }
//private:
//    void init_()
//    {
//        logManager_->setEventInterval( 3600 * 100 );
//        logManager_->setCallEventsAtStart(false);
//        LogManagerEvent event;
//        event.days_ = 30;
//        event.callback_ = boost::bind(&LogManagerTestDef::callback_,this, _1, _2);
//        event.finish_ = boost::bind(&LogManagerTestDef::finish_,this);
//        logManager_->addEvent(event);
//
//    }
//
//    void callback_(const izenelib::util::UString& query, const std::vector<std::pair<uint32_t, uint32_t> >& frequency)
//    {
//        list_.push_back( query );
//    }
//
//    void finish_()
//    {
//
//    }
//
//
//
//private:
//    LogManager* logManager_;
//    std::vector<izenelib::util::UString> list_;
//    uint32_t count_;

};

