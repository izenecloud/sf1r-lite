#include "RecommendEngineWrapper.h"
#include "Rebuilder.h"
#include <util/ustring/UString.h>

namespace sf1r
{
std::string RecommendEngineWrapper::system_resource_path_;
std::string RecommendEngineWrapper::system_working_path_;

RecommendEngineWrapper::RecommendEngineWrapper()
    : engine_(NULL)
{
    engine_ = new Recommend::RecommendEngine(system_working_path_ + "/query_correction/");
    engine_->setTokenizer(Recommend::Tokenize::getTokenizer_());
    if (engine_->isNeedBuild(system_resource_path_ + "/query_correction/"))
        engine_->buildEngine(system_resource_path_ + "/query_correction/");
    
    static Recommend::RebuildHanlder handler = boost::bind(&RecommendEngineWrapper::rebuild, this);
    Rebuilder::get()->addRebuildHanlder(&handler);
    Rebuilder::get()->start(system_resource_path_ + "/query_correction/");
}

RecommendEngineWrapper::~RecommendEngineWrapper()
{
    if (NULL != engine_)
    {
        delete engine_;
        engine_ = NULL;
    }
}

RecommendEngineWrapper& RecommendEngineWrapper::getInstance()
{
    static RecommendEngineWrapper wrapper;
    return wrapper;
}

void RecommendEngineWrapper::recommend(const izenelib::util::UString& userQuery, const uint32_t N, std::deque<izenelib::util::UString>& results) 
{
    if (NULL == engine_)
        return;
    
    boost::shared_lock<boost::shared_mutex> sl(mtx_, boost::try_to_lock);
    if (!sl)
        return;
    
    std::string str;
    userQuery.convertString(str, izenelib::util::UString::UTF_8);
    std::vector<std::string> recommends;
    engine_->recommend(str, N, recommends);
    for (std::size_t i = 0; i < recommends.size(); i++)
    {
        izenelib::util::UString uq(recommends[i], izenelib::util::UString::UTF_8);
        results.push_back(uq);
    }
}

void RecommendEngineWrapper::recommend(const std::string& userQuery, const uint32_t N, std::vector<std::string>& results) 
{
    if (NULL == engine_)
        return;
    
    boost::shared_lock<boost::shared_mutex> sl(mtx_, boost::try_to_lock);
    if (!sl)
        return;
    
    engine_->recommend(userQuery, N, results);
}

void RecommendEngineWrapper::rebuild()
{
    boost::unique_lock<boost::shared_mutex> ul(mtx_);
    if (engine_->isNeedBuild(system_resource_path_ + "/query_correction/"))
        engine_->buildEngine(system_resource_path_ + "/query_correction/");
}

}
