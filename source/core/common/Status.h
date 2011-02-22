#ifndef INCLUDE_SF1V5_COMMON_STATUS_H
#define INCLUDE_SF1V5_COMMON_STATUS_H
/**
 * @file include/common/Status.h
 * @author Ian Yang
 * @date Created <2010-07-07 10:27:52>
 */
#include <ctime>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/serialization/access.hpp>

namespace sf1r {
class StatusModificationGuard;

class Status
{
public:
    typedef StatusModificationGuard Guard;

    Status()
    : numDocs_(0), progress_(0.0f),
      elapsedTime_(0,0,0,0), leftTime_(0,0,0,0),
      running_(false), lastModified_(std::time(0)),
      counter_(0)
    {}

    bool running() const
    {
        return running_;
    }

    std::time_t lastModified() const
    {
        return lastModified_;
    }

    std::string getElapsedTime() const
    {
        return boost::posix_time::to_simple_string(elapsedTime_);
    }

    std::string getLeftTime() const
    {
        return boost::posix_time::to_simple_string(leftTime_);
    }

    std::size_t counter() const
    {
        return counter_;
    }

    /// @brief Begin modification
    void start()
    {
        running_ = true;
        // progress, elapsedTime, leftTime is valid only when
        // running is set to true.
        progress_ = 0.0f;
        elapsedTime_ = boost::posix_time::seconds(0);
        leftTime_ = boost::posix_time::hours(24);
    }

    /// @brief Commit modification
    ///
    /// Counter is changed. Modification time is also changed. If the time is
    /// very short and less then 1 seconds, it is force to add one second to the
    /// modification time.
    void commit()
    {
        running_ = false;
        ++counter_;

        std::time_t maybeModifiedTime = std::time(0);
        if (maybeModifiedTime > lastModified_)
        {
            lastModified_ = maybeModifiedTime;
        }
        else
        {
            ++lastModified_;
        }
    }

    /// @brief Cancel modification
    ///
    /// Counter is changed, but modification time is not. It indicates a
    /// failure.
    void cancel()
    {
        running_ = false;
        ++counter_;
    }

    size_t numDocs_;
    float progress_;
    boost::posix_time::time_duration elapsedTime_;
    boost::posix_time::time_duration leftTime_;
    std::string metaInfo_;

private:
    bool running_;
    std::time_t lastModified_;
    std::size_t counter_;
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive& ar, const unsigned version)
    {
        ar & running_  & progress_ & elapsedTime_ & leftTime_ & metaInfo_ & lastModified_ & numDocs_ & counter_ ;
    }
};

class StatusModificationGuard
{
public:
    explicit StatusModificationGuard(Status& status)
    : status_(&status)
    {
        status_->start();
    }

    ~StatusModificationGuard()
    {
        // not canceled? commit it
        if (status_->running())
        {
            status_->commit();
        }
    }

    void cancel()
    {
        status_->cancel();
    }

private:
    Status* status_;
};

} // namespace sf1r

#endif // INCLUDE_SF1V5_COMMON_STATUS_H
