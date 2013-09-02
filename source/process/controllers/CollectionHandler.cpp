/**
 * @file process/controllers/CollectionHandler.cpp
 * @author Ian Yang
 * @date Created <2011-01-25 18:40:12>
 */
#include <process/common/XmlConfigParser.h>
#include <process/common/CollectionManager.h>

#include <bundles/index/IndexSearchService.h>
#include <bundles/index/IndexTaskService.h>
#include <bundles/mining/MiningSearchService.h>
#include <bundles/mining/MiningTaskService.h>
#include <bundles/recommend/RecommendTaskService.h>
#include <bundles/recommend/RecommendSearchService.h>
#include <aggregator-manager/GetRecommendWorker.h>
#include <aggregator-manager/UpdateRecommendWorker.h>
#include <node-manager/DistributeRequestHooker.h>
#include <mining-manager/query-abbreviation/RemoveKeywords.h>

#include "CollectionHandler.h"
#include "DocumentsGetHandler.h"
#include "DocumentsSearchHandler.h"

#include <common/Keys.h>
#include <common/JobScheduler.h>

#include <boost/bind.hpp>

namespace sf1r
{

using driver::Keys;

using namespace izenelib::driver;
using namespace izenelib::osgi;

CollectionHandler::CollectionHandler(const string& collection)
        : collection_(collection)
        , indexSearchService_(0)
        , indexTaskService_(0)
        , productSearchService_(0)
        , productTaskService_(0)
        , miningSearchService_(0)
        , miningTaskService_(0)
        , recommendTaskService_(0)
        , recommendSearchService_(0)
        , getRecommendWorker_(0)
        , updateRecommendWorker_(0)
{
}


CollectionHandler::~CollectionHandler()
{
}

void CollectionHandler::search(::izenelib::driver::Request& request, ::izenelib::driver::Response& response)
{
    DocumentsSearchHandler handler(request,response,*this);
    handler.search();
    if (response.success() && (0 == asInt(response[Keys::total_count])))
    {
        std::string keywords = asString(request[Keys::search][Keys::keywords]);
        int toSuccess = 0;
        if (request[Keys::search].hasKey("query_abbreviation"))
            toSuccess = asInt(request[Keys::search]["query_abbreviation"]) - 1;
        else
            toSuccess = 2;
        RK::TokenRecommended queries;
        RK::queryAbbreviation(queries, keywords, *(miningSearchService_->GetMiningManager()));
        int success = 0;
        bool isLastSuccess = false;
        while (true)
        {
            const std::string& query = queries.nextToken(isLastSuccess);
            if (RK::INVALID == query)
                break;
            request[Keys::search][Keys::keywords] = query;
            ::izenelib::driver::Response newResponse;
            DocumentsSearchHandler sHandler(request, newResponse, *this);
            sHandler.search();
            if (response.success() && (0 != asInt(newResponse[Keys::total_count])))
            {
                isLastSuccess = true;
                ::izenelib::driver::Value& value = response["removed_keywords"]();
                newResponse["new_query"] = query;
                value.assign(newResponse.get());
                if (++success > toSuccess)
                    break;
            }
            else
                isLastSuccess = false;
        }
    }
}

void CollectionHandler::get(::izenelib::driver::Request& request, ::izenelib::driver::Response& response)
{
    DocumentsGetHandler handler(request, response,*this);
    handler.get();
}

void CollectionHandler::similar_to(::izenelib::driver::Request& request, ::izenelib::driver::Response& response)
{
    DocumentsGetHandler handler(request, response,*this);
    handler.similar_to();
}

void CollectionHandler::duplicate_with(::izenelib::driver::Request& request, ::izenelib::driver::Response& response)
{
    DocumentsGetHandler handler(request, response,*this);
    handler.duplicate_with();
}

void CollectionHandler::similar_to_image(::izenelib::driver::Request& request, ::izenelib::driver::Response& response)
{
    DocumentsGetHandler handler(request, response,*this);
    handler.similar_to_image();
}

bool CollectionHandler::create(const ::izenelib::driver::Value& document)
{
    //if(!indexTaskService_->HookDistributeRequest(false))
    //    return false;
    if (DistributeRequestHooker::get()->getHookType() != Request::FromAPI)
    {
        return indexTaskService_->createDocument(document);
    }
    task_type task = boost::bind(&IndexTaskService::createDocument, indexTaskService_, document);
    JobScheduler::get()->addTask(task, collection_);
    return true;
}

bool CollectionHandler::update(const ::izenelib::driver::Value& document)
{
    if (DistributeRequestHooker::get()->getHookType() != Request::FromAPI)
    {
        return indexTaskService_->updateDocument(document);
    }
    task_type task = boost::bind(&IndexTaskService::updateDocument, indexTaskService_, document);
    JobScheduler::get()->addTask(task, collection_);
    return true;
}

bool CollectionHandler::update_inplace(const ::izenelib::driver::Value& request)
{
    if (DistributeRequestHooker::get()->getHookType() != Request::FromAPI)
    {
        return indexTaskService_->updateDocumentInplace(request);
    }
    task_type task = boost::bind(&IndexTaskService::updateDocumentInplace, indexTaskService_, request);
    JobScheduler::get()->addTask(task, collection_);
    return true;
}

bool CollectionHandler::destroy(const ::izenelib::driver::Value& document)
{
    if (DistributeRequestHooker::get()->getHookType() != Request::FromAPI)
    {
        return indexTaskService_->destroyDocument(document);
    }
    task_type task = boost::bind(&IndexTaskService::destroyDocument, indexTaskService_, document);
    JobScheduler::get()->addTask(task, collection_);
    return true;
}

} // namespace sf1r
