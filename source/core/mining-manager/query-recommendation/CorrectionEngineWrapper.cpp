#include "CorrectionEngineWrapper.h"
#include "pinyin/PinYin.h"
#include <sys/inotify.h>

namespace sf1r
{

std::string CorrectionEngineWrapper::system_resource_path_;
std::string CorrectionEngineWrapper::system_working_path_;

CorrectionEngineWrapper::CorrectionEngineWrapper()
    : engine_(NULL)
    , monitor_(NULL)
{
    engine_ = new Recommend::CorrectionEngine(system_working_path_ + "/query_correction/");
    engine_->setPinYinConverter(Recommend::PinYin::QueryCorrection::getInstance().getPinYinConverter());
    engine_->setPinYinApproximator(Recommend::PinYin::QueryCorrection::getInstance().getPinYinApproximator());
    if (engine_->isNeedBuild(system_resource_path_ + "/query_correction/"))
        engine_->buildEngine(system_resource_path_ + "/query_correction/");
    
    monitor_ = new ResourceMonitor(system_resource_path_ + "/query_correction", IN_ATTRIB);
    static ResourceHandler handler = boost::bind(&CorrectionEngineWrapper::rebuild, this, _1, _2);
    monitor_->setResourceHandler(&handler);
    monitor_->monitor();

}

CorrectionEngineWrapper:: ~CorrectionEngineWrapper()
{
    if (NULL != monitor_)
    {
        delete monitor_;
        monitor_ = NULL;
    }
    if (NULL != engine_)
    {
        delete engine_;
        engine_ = NULL;
    }
}

void CorrectionEngineWrapper::rebuild(const std::string& dir, uint32_t mask)
{
    boost::unique_lock<boost::shared_mutex> ul(mtx_);
    if (engine_->isNeedBuild(dir))
        engine_->buildEngine(dir);
}

CorrectionEngineWrapper& CorrectionEngineWrapper::getInstance()
{
    static CorrectionEngineWrapper wrapper;
    return wrapper;
}

bool CorrectionEngineWrapper::correct(const std::string& userQuery, std::string& results, double& freq )
{
    boost::shared_lock<boost::shared_mutex> sl(mtx_, boost::try_to_lock);

    if (!sl)
        return false;

    if (NULL != engine_)
        return engine_->correct(userQuery, results, freq);
    return false;
}
    
void CorrectionEngineWrapper::evaluate(std::string& stream)
{
    boost::shared_lock<boost::shared_mutex> sl(mtx_, boost::try_to_lock);
    if (!sl)
        return ;

    if (NULL != engine_)
        engine_->evaluate(system_resource_path_ + "/query_correction/evaluate/", stream);
}

}
