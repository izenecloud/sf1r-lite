#include "AdFeedbackMgr.h"

#define USER_PROFILE_CACHE_SECS 60

namespace sf1r
{

AdFeedbackMgr::AdFeedbackMgr()
{
}

AdFeedbackMgr::~AdFeedbackMgr()
{
}

void AdFeedbackMgr::init(const std::string& dmp_server_ip, uint16_t port)
{
    dmp_server_ip_ = dmp_server_ip;
    dmp_server_port_ = port;
}

bool AdFeedbackMgr::getUserProfileFromDMP(const std::string& user_id, UserProfile& user_profile)
{
    if (cached_user_profiles_.getValueNoInsert(user_id, user_profile))
    {
        if (std::time(NULL) < (user_profile.timestamp + USER_PROFILE_CACHE_SECS))
        {
            return true;
        }
    }
    // connect to the DMP
    //
    // get the user profile by user id.
    return true;
}

bool AdFeedbackMgr::parserFeedbackLog(const std::string& log_data, FeedbackInfo& feedback_info)
{
    // extract the user id, ad id and action from log
    //
    //
    return getUserProfileFromDMP(feedback_info.user_id, feedback_info.user_profiles);
}

}

