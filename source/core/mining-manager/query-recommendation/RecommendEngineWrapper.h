#ifndef RECOMMEND_ENGINE_WRAPPER_H
#define RECOMMEND_ENGINE_WRAPPER_H

#include "RecommendEngine.h"

namespace sf1r
{
class RecommendEngineWrapper : public boost::noncopyable
{
public:
    RecommendEngineWrapper();
    ~RecommendEngineWrapper();
    static RecommendEngineWrapper& getInstance();

public:
    void recommend(const izenelib::util::UString& userQuery, const uint32_t N, std::deque<izenelib::util::UString>& results);
    void recommend(const std::string& userQuery, const uint32_t N, std::vector<std::string>& results); 
    void rebuild();
public:
    static std::string system_resource_path_;
    static std::string system_working_path_;

private:
    Recommend::RecommendEngine* engine_;
    boost::shared_mutex mtx_;
};
}

#endif
