/**
 * \file CollectionController.cpp
 * \brief 
 * \date Dec 20, 2011
 * \author Xin Liu
 */

#include "CollectionController.h"
#include "CollectionHandler.h"
#include "process/common/XmlSchema.h"
#include "core/license-manager/LicenseCustManager.h"
#include <node-manager/MasterManagerBase.h>
#include <node-manager/NodeManagerBase.h>
#include <node-manager/RequestLog.h>
#include <node-manager/DistributeRequestHooker.h>
#include <node-manager/RecoveryChecker.h>
#include <util/driver/writers/JsonWriter.h>

#include <process/common/CollectionManager.h>
#include <bundles/mining/MiningSearchService.h>
#include <common/CollectionTask.h>
#include <common/CollectionTaskScheduler.h>
#include <iostream>
#include <fstream>

namespace sf1r
{

bool CollectionController::callDistribute()
{
    if(request().callType() != Request::FromAPI)
        return false;
    if (MasterManagerBase::get()->isDistributed())
    {
        bool is_write_req = ReqLogMgr::isWriteRequest(request().controller(), request().action());
        if (is_write_req)
        {
            std::string reqdata;
            izenelib::driver::JsonWriter writer;
            writer.write(request().get(), reqdata);
            if(!MasterManagerBase::get()->pushWriteReq(reqdata))
                response().addError("push write request to queue failed.");
            return true;
        }
    }
    return false;

}

bool CollectionController::preprocess()
{
    // need call in distribute, so we push it to queue and return false to ignore this request.
    if(callDistribute())
        return false;
    if (request().callType() == Request::FromAPI)
    {
        // from api do not need hook, just process as usually. all read only request 
        // will return from here.
        return true;
    }
    //const std::string& reqdata = DistributeRequestHooker::get()->getAdditionData();
    //DistributeRequestHooker::get()->hookCurrentReq(reqdata);
    //DistributeRequestHooker::get()->processLocalBegin();
    // note: if the request need to shard to different node.
    // you should do hook again to all shard before you actually do the processing.
    return true;

}

void CollectionController::postprocess()
{
    if (request().callType() != Request::FromAPI)
    {
        if (!response().success())
        {
            DistributeRequestHooker::get()->processLocalFinished(false);
            std::string errinfo;
            try
            {
                errinfo = toJsonString(response()[Keys::errors]);
            }
            catch(...)
            {
            }
            std::cout << "request failed before send!!" << errinfo << std::endl;
        }
        else
        {
            //if (request().callType() == Request::FromDistribute &&
            //    DistributeRequestHooker::isAsyncWriteRequest(request().controller(), request().action()))
            //{
            //    std::cout << " a async request finish send, may not finished actually. Leave for async handler." << std::endl;
            //    return;
            //}
            std::cout << " sync request finished success. calltype: " << request().callType()
                << ", api: " << request().controller() << "_" << request().action() << std::endl;
            DistributeRequestHooker::get()->processLocalFinished(true);
        }
    }
}

/**
 * @brief Action @b start_collection.
 *
 * @section request
 *
 * - @b collection* (@c String): Collection name.
 *
 * @section response
 *
 * - No extra fields.
 *
 * @section example
 *
 * Request
 *
 * @code
 * {
 *   "collection": "chwiki"
 * }
 * @endcode
 *
 */
void CollectionController::start_collection()
{
    std::string collection = asString(request()[Keys::collection]);
    if (collection.empty())
    {
        response().addError("Require field collection in request.");
        return;
    }

    CollectionHandler* collectionHandler = CollectionManager::get()->findHandler(collection);
    if (collectionHandler )
    {
        response().addError("Collection has already started!");
        return;
    }

    std::string configFile = SF1Config::get()->getHomeDirectory();
    std::string slash("");
#ifdef WIN32
        slash = "\\";
#else
        slash = "/";
#endif
    configFile += slash + collection + ".xml";

    DISTRIBUTE_WRITE_BEGIN;
    DISTRIBUTE_WRITE_CHECK_VALID_RETURN2;

    UpdateConfigReqLog reqlog;
    if (DistributeRequestHooker::get()->isRunningPrimary())
    {
        LOG(INFO) << "starting collection on primary. " << collection; 
        if (!bfs::exists(configFile))
        {
            LOG(ERROR) << "starting collection on primary failed, no configFile :" << configFile;
            response().addError("start collection failed for no config file found.");
            return;
        }
    }
    if(!DistributeRequestHooker::get()->prepare(Req_UpdateConfig, reqlog))
    {
        LOG(ERROR) << "prepare failed in " << __FUNCTION__;
        response().addError("prepare failed.");
        return;
    }

    bool ret = RecoveryChecker::get()->updateConfigFromAPI(collection,
        DistributeRequestHooker::get()->isRunningPrimary(), configFile, reqlog.config_file_list);

    if (!ret)
    {
        response().addError("Update config error while starting collection.");
        return;
    }

    ret = CollectionManager::get()->startCollection(collection, configFile);
    if (!ret)
    {
        response().addError("start collection failed.");
        return;
    }

    if (SF1Config::get()->isDistributedNode())
    {
        NodeManagerBase::get()->updateTopologyCfg(SF1Config::get()->topologyConfig_.sf1rTopology_);
    }

    DISTRIBUTE_WRITE_FINISH2(ret, reqlog);
}

/**
 * @brief Action @b stop_collection.
 *
 * @section request
 *
 * - @b collection* (@c String): Collection name.
 *
 * @section response
 *
 * - No extra fields.
 *
 * @section example
 *
 * Request
 *
 * @code
 * {
 *   "collection": "chwiki"
 * }
 * @endcode
 *
 */
void CollectionController::stop_collection()
{
    std::string collection = asString(request()[Keys::collection]);
    bool clear = false;
    if(!izenelib::driver::nullValue( request()[Keys::clear] ) )
    {
        clear = asBool(request()[Keys::clear]);
    }
    if (collection.empty())
    {
        response().addError("Require field collection in request.");
        return;
    }
    if (!SF1Config::get()->checkCollectionAndACL(collection, request().aclTokens()))
    {
        response().addError("Collection access denied");
        return;
    }

    CollectionHandler* collectionHandler = CollectionManager::get()->findHandler(collection);
    if (!collectionHandler )
    {
        response().addError("Collection not found!");
        return;
    }

    DISTRIBUTE_WRITE_BEGIN;
    DISTRIBUTE_WRITE_CHECK_VALID_RETURN2;

    NoAdditionReqLog reqlog;
    if(!DistributeRequestHooker::get()->prepare(Req_NoAdditionDataReq, reqlog))
    {
        LOG(ERROR) << "prepare failed in " << __FUNCTION__;
        response().addError("prepare failed.");
        return;
    }

    LOG(INFO) << "begin stopping collection : " << collection;

    bool ret = CollectionManager::get()->stopCollection(collection, clear);
    if (!ret)
    {
        response().addError("stop collection failed.");
        return;
    }

    RecoveryChecker::get()->removeConfigFromAPI(collection);

    if (SF1Config::get()->isDistributedNode())
    {
        NodeManagerBase::get()->updateTopologyCfg(SF1Config::get()->topologyConfig_.sf1rTopology_);
    }

    DISTRIBUTE_WRITE_FINISH(ret);
}
/**
 * @brief Action @b check_collection. Used for consistency check for distribute.
 *
 * @section request
 *
 * - @b collection* (@c String): Collection name.
 *
 * @section response
 *
 * - No extra fields.
 *
 * @section example
 *
 * Request
 *
 * @code
 * {
 *   "collection": "chwiki"
 * }
 * @endcode
 *
 */
void CollectionController::check_collection()
{
    std::string collection = asString(request()[Keys::collection]);
    if (collection.empty())
    {
        response().addError("Require field collection in request.");
        return;
    }
    if (!SF1Config::get()->checkCollectionAndACL(collection, request().aclTokens()))
    {
        response().addError("Collection access denied");
        return;
    }
    std::string errinfo = CollectionManager::get()->checkCollectionConsistency(collection);
    if (!errinfo.empty())
    {
        response().addError(errinfo);
    }
}

void CollectionController::update_collection_conf()
{
    std::string collection = asString(request()[Keys::collection]);
    if (collection.empty())
    {
        response().addError("Require field collection in request.");
        return;
    }

    if (!SF1Config::get()->checkCollectionAndACL(collection, request().aclTokens()))
    {
        response().addError("Collection access denied");
        return;
    }

    CollectionHandler* collectionHandler = CollectionManager::get()->findHandler(collection);
    if (!collectionHandler )
    {
        response().addError("Collection not found!");
        return;
    }

    if (!MasterManagerBase::get()->isDistributed())
    {
        response().addError("This api only available in distributed mode.");
        return;
    }

    bool ret = true;
    DISTRIBUTE_WRITE_BEGIN;
    DISTRIBUTE_WRITE_CHECK_VALID_RETURN2;

    if(!CollectionManager::get()->stopCollection(collection, false))
    {
        LOG(ERROR) << "failed to stop collection while update config." << collection;
        response().addError("failed to stop collection while update config.");
        return;
    }

    std::string configFile = SF1Config::get()->getHomeDirectory();
    std::string slash("");
#ifdef WIN32
        slash = "\\";
#else
        slash = "/";
#endif
    configFile += slash + collection + ".xml";

    UpdateConfigReqLog reqlog;
    do {
        if (!DistributeRequestHooker::get()->prepare(Req_UpdateConfig, reqlog))
        {
            ret = false;
            break;
        }
        ret = RecoveryChecker::get()->updateConfigFromAPI(collection,
            DistributeRequestHooker::get()->isRunningPrimary(), configFile, reqlog.config_file_list);
    }while(false);

    if (!ret)
    {
        response().addError("Update Config failed.");
        return;
    }

    ret = CollectionManager::get()->startCollection(collection, configFile);
    if (!ret)
    {
        LOG(ERROR) << "start collection failed after config updated." << collection;
        response().addError("start collection failed after config updated.");
        return;
    }

    if (SF1Config::get()->isDistributedNode())
    {
        NodeManagerBase::get()->updateTopologyCfg(SF1Config::get()->topologyConfig_.sf1rTopology_);
    }

    DISTRIBUTE_WRITE_FINISH2(ret, reqlog);
}

void CollectionController::backup_all()
{
    if (!MasterManagerBase::get()->isDistributed())
    {
        response().addError("This api only available in distributed mode.");
        return;
    }
    bool force_backup = false;
    force_backup = asBool(request()[Keys::force_backup]);

    DISTRIBUTE_WRITE_BEGIN;
    DISTRIBUTE_WRITE_CHECK_VALID_RETURN2;

    NoAdditionNoRollbackReqLog reqlog;
    if (!DistributeRequestHooker::get()->prepare(Req_NoAdditionDataNoRollback, reqlog))
    {
        response().addError("Backup prepared failed.");
        return;
    }

    bool ret = CollectionManager::get()->backup_all(force_backup);
    if(!ret)
    {
        response().addError("Backup all failed.");
        return;
    }

    DISTRIBUTE_WRITE_FINISH(ret);
}


/**
 * @brief Action @b rebuild_from_scd. Clean old data and rebuild new data from full scd files.
 *  please put the full scd files to the specific directory on primary node.
 *
 * @section request
 *
 * - @b collection* (@c String): Collection name.
 *
 * @section response
 *
 * - No extra fields.
 *
 * @section example
 *
 * Request
 *
 * @code
 * {
 *   "collection": "chwiki"
 * }
 * @endcode
 *
 */
void CollectionController::rebuild_from_scd()
{
    std::string collection = asString(request()[Keys::collection]);
    if (collection.empty())
    {
        response().addError("Require field collection in request.");
        return;
    }
    if (!SF1Config::get()->checkCollectionAndACL(collection, request().aclTokens()))
    {
        response().addError("Collection access denied");
        return;
    }
    CollectionHandler* collectionHandler = CollectionManager::get()->findHandler(collection);
    if (!collectionHandler || !collectionHandler->indexTaskService_)
    {
        response().addError("Collection not found or no index service!");
        return;
    }

    if (!MasterManagerBase::get()->isDistributed())
    {
        response().addError("This api only available in distributed mode.");
        return;
    }
    // total clean rebuild .
    boost::shared_ptr<RebuildTask> task(new RebuildTask(collection));
    if (!task->rebuildFromSCD(asString(request()[Keys::index_scd_path])))
    {
        response().addError("Rebuild from scd failed.");
        return;
    }
}

/**
 * @brief Action @b rebuild_collection. To clear these deleted Document;
 *
 * @section request
 *
 * - @b collection* (@c String): Collection name.
 *
 * @section response
 *
 * - No extra fields.
 *
 * @section example
 *
 * Request
 *
 * @code
 * {
 *   "collection": "chwiki"
 * }
 * @endcode
 *
 */
void CollectionController::rebuild_collection()
{
    std::string collection = asString(request()[Keys::collection]);
    if (collection.empty())
    {
        response().addError("Require field collection in request.");
        return;
    }
    if (!SF1Config::get()->checkCollectionAndACL(collection, request().aclTokens()))
    {
        response().addError("Collection access denied");
        return;
    }
    CollectionHandler* collectionHandler = CollectionManager::get()->findHandler(collection);
    if (!collectionHandler || !collectionHandler->indexTaskService_)
    {
        response().addError("Collection not found or no index service!");
        return;
    }

    boost::shared_ptr<RebuildTask> task(new RebuildTask(collection));
    if (MasterManagerBase::get()->isDistributed())
    {
        if(!MasterManagerBase::get()->pushWriteReq("CollectionTaskScheduler-" + task->getTaskName(), "cron"))
        {
            response().addError("push rebuild task failed, maybe the auto rebuild not enabled.!");
            return;
        }
        LOG(INFO) << "push rebuild cron job to queue from api: " << "CollectionTaskScheduler-" + task->getTaskName();
    }
    else
        task->doTask();
}
/**
 * @brief Action @b create_collection.
 *
 * @section request
 *
 * - @b collection* (@c String): Collection name.
 * - @b collection_config* (@c String): Config file string.
 *
 * @section response
 *
 * - No extra fields.
 *
 * @section example
 *
 * Request
 *
 * @code
 * {
 *   "collection": "chwiki"
 *   "collection_config": "xxxxxx"
 * }
 * @endcode
 *
 */
void CollectionController::create_collection()
{
    namespace bf = boost::filesystem;
    std::string collection = asString(request()[Keys::collection]);
    std::string config = asString(request()[Keys::collection_config]);

    std::string configFile = SF1Config::get()->getHomeDirectory();
    std::string slash("");
#ifdef WIN32
            slash = "\\";
#else
            slash = "/";
#endif
    configFile += slash + collection + ".xml";
    if(bf::exists(configFile)){
        response().addError("Collection already exists");
        return;
    }

    if (MasterManagerBase::get()->isDistributed())
    {
        response().addError(" Request not allowed in distributed node.");
        return;
    }

    bf::path configPath(configFile);
    ofstream config_file(configPath.string().c_str(), ios::out);
    if(!config_file){
        response().addError("Can't create config file");
        return;
    }

    int tab = 0;
    for(string::iterator it = config.begin(); it != config.end(); ){
        if(*it == '<'){
            char c = *it;
            it++;
            if(it != config.end()){
                if(*it == '/'){
                    tab--;
                    for(int i = 0; i < tab; ++i)
                        config_file<<"    ";
                }
                else{
                    for(int i = 0; i < tab; ++i)
                        config_file<<"    ";
                    if(*it != '!' && *it != '?')
                        tab++;
                }
            }
            config_file.put(c);
        }
        else if(*it == '/'){
            config_file.put(*it);
            it++;
            if(it != config.end()){
                if(*it == '>')
                    tab--;
            }
        }
        else{
            config_file.put(*it);
            if(*it == '>')
                config_file.put('\n');
            it++;
        }
    }
    config_file.close();

    bf::path config_dir = configPath.parent_path();
    bf::path schema_file = config_dir/"schema"/"collection.xsd";
    std::string schema_file_string = schema_file.string();
    if(!bf::exists(schema_file_string)){
        response().addError("[Collection] Schema File doesn't exist");
        bf::remove(configFile);
        return;
    }
    XmlSchema schema(schema_file_string);
    bool schema_valid = schema.validate(configFile);
    std::list<std::string> schema_warning = schema.getSchemaValidityWarnings();
    if(schema_warning.size()>0){
        std::list<std::string>::iterator it = schema_warning.begin();
        while (it != schema_warning.end())
        {
            response().addWarning("[Schema-warning] " + *it);
            it++;
        }
    }
    if(!schema_valid){
        std::list<std::string> schema_error = schema.getSchemaValidityErrors();
        if(schema_error.size()>0){
            std::list<std::string>::iterator it = schema_error.begin();
            while(it != schema_error.end()){
                response().addError("[Schema-Error] " + *it);
                it++;
            }
        }
        bf::remove(configFile);
        return;
    }
    cout<<"Created collection: "<<collection<<endl;

}

/**
 * @brief Action @b delete_collection.
 *
 * @section request
 *
 * - @b collection* (@c String): Collection name.
 *
 * @section response
 *
 * - No extra fields.
 *
 * @section example
 *
 * Request
 *
 * @code
 * {
 *   "collection": "chwiki"
 * }
 * @endcode
 *
 */
void CollectionController::delete_collection(){
    namespace bf = boost::filesystem;
    std::string collection = asString(request()[Keys::collection]);

    if(CollectionManager::get()->findHandler(collection) != NULL){
        response().addError("Collection is not stopped, can't be deleted.");
        return;
    }

    if (MasterManagerBase::get()->isDistributed())
    {
        response().addError(" Request not allowed in distributed node.");
        return;
    }
    std::string configFile = SF1Config::get()->getHomeDirectory();

    std::string slash("");
#ifdef WIN32
            slash = "\\";
#else
            slash = "/";
#endif
    configFile += slash + collection + ".xml";
    if(!bf::exists(configFile)){
        response().addError("Collection does not exists");
        return;
    }

    bf::remove(configFile);
    
    bf::path collection_dir("collection" + slash + collection);
    if(bf::exists(collection_dir)){
        if(bf::is_empty(collection_dir))
            bf::remove(collection_dir);
        else
            bf::remove_all(collection_dir);
    }
    cout<<"deleted collection: "<<collection<<endl;
}


/**
 * @brief Action @b set_kv.
 *
 * @section request
 *
 * - @b collection* (@c String): Collection name.
 * - @b key* (@c String): key.
 * - @b value* (@c String): value.
 *
 * @section response
 *
 * - No extra fields.
 *
 * @section example
 *
 * Request
 *
 * @code
 * {
 *   "collection": "b5mp",
 *   "key": "x",
 *   "value": "y"
 * }
 * @endcode
 *
 */
void CollectionController::set_kv()
{
    std::string collection = asString(request()[Keys::collection]);
    std::string key = asString(request()[Keys::key]);
    std::string value = asString(request()[Keys::value]);
    if (collection.empty() || key.empty() || value.empty())
    {
        response().addError("Require field collection,key,value in request.");
        return;
    }
    CollectionHandler* handler = CollectionManager::get()->findHandler(collection);
    if(handler==NULL)
    {
        response().addError("collection not found");
        return;
    }

    bool ret = handler->miningSearchService_->SetKV(key, value);
    if(!ret)
    {
        response().addError("set kv fail");
    }
}

/**
 * @brief Action @b get_kv.
 *
 * @section request
 *
 * - @b collection* (@c String): Collection name.
 * - @b key* (@c String): key.
 *
 * @section response
 *
 * - @b value (@c String): return value
 *
 * @section example
 *
 * Request
 *
 * @code
 * {
 *   "collection": "b5mp",
 *   "key": "x",
 * }
 * @endcode
 *
 */
void CollectionController::get_kv()
{
    std::string collection = asString(request()[Keys::collection]);
    std::string key = asString(request()[Keys::key]);
    if (collection.empty() || key.empty())
    {
        response().addError("Require field collection,key in request.");
        return;
    }
    CollectionHandler* handler = CollectionManager::get()->findHandler(collection);
    if(handler==NULL)
    {
        response().addError("collection not found");
        return;
    }
    std::string value;
    if(handler->miningSearchService_->GetKV(key, value))
    {
        response()[Keys::value] = value;
    }
}

/**
 * @brief Action @b load_license.
 *
 * @section request
 *
 * - @b path (@c String): license file path.
 *
 * @section response
 *
 * - No extra fields.
 *
 * @section example
 *
 * Request
 *
 * @code
 * {
 *	  "path": "/home/customer/sf1_license/license_key.dat"
 * }
 * @endcode
 *
 */

void CollectionController::load_license()
{
	std::string path = asString(request()[Keys::path]);
	if (path.empty())
	{
		response().addError("Require field path in request.");
		return;
	}

	std::vector<std::string> collectionList;
	LicenseCustManager::get()->setLicenseFilePath(path);
	LicenseCustManager::get()->setCollectionInfo();
	collectionList = LicenseCustManager::get()->getCollectionList();

	if (collectionList.empty())
	{
		response().addError("There's no information about customer in your license file.");
		return;
	}
	std::string configFilePath = getConfigPath_();
	std::string slash("");
#ifdef WIN32
		slash = "\\";
#else
		slash = "/";
#endif
    std::vector<std::string>::const_iterator iter;
    for(iter = collectionList.begin(); iter != collectionList.end(); iter++)
    {
    	std::string collectionName = *iter;
    	std::string configFile = configFilePath + slash + collectionName + ".xml";
    	CollectionManager::get()->startCollection(collectionName, configFile);
    	CollectionTaskScheduler::get()->scheduleLicenseTask(collectionName);
    }
}

} //namespace sf1r

