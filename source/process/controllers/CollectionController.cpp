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
#include <node-manager/RequestLog.h>
#include <node-manager/DistributeRequestHooker.h>
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
            MasterManagerBase::get()->pushWriteReq(reqdata);
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
    // hook request if needed.
    Request::kCallType hooktype = (Request::kCallType)DistributeRequestHooker::get()->getHookType();
    if (hooktype == Request::FromAPI)
    {
        // from api do not need hook, just process as usually. all read only request 
        // will return from here.
        return true;
    }
    const std::string& reqdata = DistributeRequestHooker::get()->getAdditionData();
    DistributeRequestHooker::get()->hookCurrentReq(reqdata);
    DistributeRequestHooker::get()->processLocalBegin();
    // note: if the request need to shard to different node.
    // you should do hook again to all shard before you actually do the processing.
    return true;

}

void CollectionController::postprocess()
{
    if (!response().success() && DistributeRequestHooker::get()->isHooked())
    {
        DistributeRequestHooker::get()->processFailedBeforePrepare();
        std::string errinfo;
        try
        {
            errinfo = response()[Keys::errors].get<std::string>();
        }
        catch(...)
        {
        }
        std::cout << "request failed before send!!" << errinfo << std::endl;
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

    if (!DistributeRequestHooker::get()->isValid())
    {
        LOG(ERROR) << __FUNCTION__ << " call invalid.";
        return;
    }
    NoAdditionReqLog reqlog;
    if(!DistributeRequestHooker::get()->prepare(Req_NoAdditionDataReq, reqlog))
    {
        LOG(ERROR) << "prepare failed in " << __FUNCTION__;
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
    bool ret = CollectionManager::get()->startCollection(collection, configFile);

    DistributeRequestHooker::get()->processLocalFinished(ret);
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

    if (!DistributeRequestHooker::get()->isValid())
    {
        LOG(ERROR) << __FUNCTION__ << " call invalid.";
        return;
    }
    NoAdditionReqLog reqlog;
    if(!DistributeRequestHooker::get()->prepare(Req_NoAdditionDataReq, reqlog))
    {
        LOG(ERROR) << "prepare failed in " << __FUNCTION__;
        return;
    }

    bool ret = CollectionManager::get()->stopCollection(collection, clear);

    DistributeRequestHooker::get()->processLocalFinished(ret);
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

    if (!DistributeRequestHooker::get()->isValid())
    {
        LOG(ERROR) << __FUNCTION__ << " call invalid.";
        return;
    }
    NoAdditionNeedBackupReqLog reqlog;
    if(!DistributeRequestHooker::get()->prepare(Req_NoAdditionData_NeedBackup_Req, reqlog))
    {
        LOG(ERROR) << "prepare failed in " << __FUNCTION__;
        return;
    }


    boost::shared_ptr<RebuildTask> task(new RebuildTask(collection));
    task->doTask();

    DistributeRequestHooker::get()->processLocalFinished(true);
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

