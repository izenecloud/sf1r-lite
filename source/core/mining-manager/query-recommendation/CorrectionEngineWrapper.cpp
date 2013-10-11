#include "CorrectionEngineWrapper.h"
#include "pinyin/PinYin.h"

namespace sf1r
{

std::string CorrectionEngineWrapper::system_resource_path_;
std::string CorrectionEngineWrapper::system_working_path_;

CorrectionEngineWrapper::CorrectionEngineWrapper()
    : engine_(NULL)
{
    engine_ = new Recommend::CorrectionEngine(system_working_path_ + "/query_correction/");
    engine_->setPinYinConverter(Recommend::getPinYinConverter());
    if (engine_->isNeedBuild(system_resource_path_ + "/query_correction/"))
        engine_->buildEngine(system_resource_path_ + "/query_correction/");
}

CorrectionEngineWrapper:: ~CorrectionEngineWrapper()
{
    if (NULL != engine_)
    {
        delete engine_;
        engine_ = NULL;
    }
}
    
CorrectionEngineWrapper& CorrectionEngineWrapper::getInstance()
{
    static CorrectionEngineWrapper wrapper;
    return wrapper;
}

bool CorrectionEngineWrapper::correct(const std::string& userQuery, std::string& results, double& freq ) const
{
    if (NULL != engine_)
        return engine_->correct(userQuery, results, freq);
    return false;
}

}
