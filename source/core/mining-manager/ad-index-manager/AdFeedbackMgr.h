#ifndef AD_FEEDBACK_MGR_H
#define AD_FEEDBACK_MGR_H

#include <util/singleton.h>
#include <boost/thread.hpp>
#include <cache/IzeneCache.h>
#include <3rdparty/msgpack/msgpack.hpp>

namespace sf1r
{

class AdFeedbackMgr
{
public:
    static AdFeedbackMgr* get()
    {
        return izenelib::util::Singleton<AdFeedbackMgr>::get();
    }

    AdFeedbackMgr();
    ~AdFeedbackMgr();

    enum FeedbackActionT
    {
        View,
        Click,
        Buy,
        TotalAction
    };

    struct UserProfile
    {
        std::map<std::string, double> profile_data;
        time_t timestamp;
        MSGPACK_DEFINE(profile_data, timestamp);
    };

    struct FeedbackInfo
    {
        std::string user_id;
        std::string ad_id;
        FeedbackActionT action;
        UserProfile user_profiles;
    };

    void init(const std::string& dmp_server_ip, uint16_t port);
    bool parserFeedbackLog(const std::string& log_data, FeedbackInfo& feedback_info);

private:
    bool getUserProfileFromDMP(const std::string& user_id, UserProfile& user_profile);

    typedef izenelib::cache::IzeneCache<std::string, UserProfile,
            izenelib::util::ReadWriteLock,
            izenelib::cache::RDE_HASH,
            izenelib::cache::LRLFU > cache_type;
    std::string dmp_server_ip_;
    uint16_t dmp_server_port_;
    cache_type cached_user_profiles_;
};

};

#endif
