#include "QueryLogBundleActivator.h"

#include <mining-manager/MiningQueryLogHandler.h>
#include <mining-manager/query-correction-submanager/QueryCorrectionSubmanager.h>
#include <mining-manager/auto-fill-submanager/AutoFillSubManager.h>
#include <util/osgi/ObjectCreator.h>

#include <boost/shared_ptr.hpp>

namespace sf1r
{

using namespace izenelib::osgi;
QueryLogBundleActivator::QueryLogBundleActivator()
    :tracker_(0)
    ,context_(0)
    ,serviceReg_(0)
    ,config_(0)
{
}

QueryLogBundleActivator::~QueryLogBundleActivator()
{
}

void QueryLogBundleActivator::start( IBundleContext::ConstPtr context )
{
    context_ = context;

    boost::shared_ptr<BundleConfiguration> bundleConfigPtr = context->getBundleConfig();
    config_ = static_cast<QueryLogBundleConfiguration*>(bundleConfigPtr.get());

    MiningQueryLogHandler* handler = MiningQueryLogHandler::getInstance();
    handler->SetParam(config_ ->update_time, config_ ->log_days);
    if ( !handler->cronStart(config_->cronIndexRecommend_) )
    {
        std::cout<<"Can not start cron job for recommend, cron_string: "<<config_->cronIndexRecommend_<<std::endl;
    }
    std::string query_support_path = config_->basepath;
    std::string query_correction_res_path = config_->resource_dir_+"/speller-support";
    // ------------------------------ [ Set path of auto fill sub manager ]
//     if( !AutoFillSubManager::get()->Init
//       ( config_->basepath+"/autofill", config_->autofill_num, 0) )
//     {
//         std::cerr<<"Autofill init failed."<<std::endl;
//         return;
//     }
    std::string query_correction_path = query_support_path+"/querycorrection";
    boost::filesystem::create_directories(query_correction_path);
    QueryCorrectionSubmanagerParam::set( query_correction_res_path
    , query_correction_path, config_->query_correction_enableEK,
                                         config_->query_correction_enableCN );
    QueryCorrectionSubmanager::getInstance();
}

void QueryLogBundleActivator::stop( IBundleContext::ConstPtr context )
{
}

bool QueryLogBundleActivator::addingService( const ServiceReference& ref )
{
    return true;
}

void QueryLogBundleActivator::removedService( const ServiceReference& ref )
{

}

REGISTER_BUNDLE_ACTIVATOR_CLASS( "QueryLogBundleActivator", QueryLogBundleActivator )

}

