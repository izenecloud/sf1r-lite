#include "AdFeedbackMgr.h"
#include <glog/logging.h>
#include <3rdparty/rapidjson/reader.h>
#include <3rdparty/rapidjson/document.h>

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
    namespace rj = rapidjson;
    // extract the user id, ad id and action from log
    rj::Document log_doc;
    bool err = log_doc.Parse<0>(log_data.c_str()).HasParseError();
    if (err)
    {
        LOG(ERROR) << "parsing json log data error.";
        return false;
    }
    if (!log_doc.HasMember("args"))
    {
        LOG(ERROR) << "log data has no args.";
        return false;
    }
    const rj::Value& args = log_doc["args"];
    if (!args.IsObject())
    {
        LOG(ERROR) << "log args is not object.";
        return false;
    }
    if (!args.HasMember("ad") ||
        !args.HasMember("uid") ||
        !args.HasMember("dstl") ||
        !args.HasMember("aid"))
    {
        // some of necessary value missing, just ignore.
        return false;
    }
    feedback_info.user_id = args["uid"].GetString();
    feedback_info.ad_id = args["aid"].GetString();
    std::string action_str = args["ad"].GetString();
    if (action_str == "103")
        feedback_info.action = Click;
    else if (action_str == "108")
        feedback_info.action = View;
    else
    {
        LOG(INFO) << "unknow action string: " << action_str;
        feedback_info.action = View;
    }
    if (feedback_info.user_id.empty() ||
        feedback_info.ad_id.empty())
    {
        LOG(INFO) << "empty user id or ad id." << feedback_info.user_id << ":" << feedback_info.ad_id;
        return false;
    }
    //
    return getUserProfileFromDMP(feedback_info.user_id, feedback_info.user_profiles);
}

}

