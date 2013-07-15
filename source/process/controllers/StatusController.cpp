/**
 * @file process/controllers/StatusController.cpp
 * @author Ian Yang
 * @date Created <2010-07-07 10:12:15>
 */
#include "StatusController.h"
#include "CollectionHandler.h"
#include <node-manager/DistributeTest.hpp>
#include <node-manager/NodeManagerBase.h>
#include <bundles/index/IndexTaskService.h>

#include <common/Status.h>
#include <common/Keys.h>

namespace sf1r
{
using driver::Keys;
using namespace izenelib::driver;

StatusController::StatusController()
    : indexTaskService_(NULL)
{
}

bool StatusController::checkCollectionService(std::string& error)
{
    indexTaskService_ = collectionHandler_->indexTaskService_;

    if (indexTaskService_)
        return true;

    error = "Request failed, no index task service found.";
    return false;
}

/**
 * @brief Action \b index. Get index and mining status of specified collection.
 *
 * To wait for index or mining finish, please first save the @c counter and @c
 * last_modified before sending index and mining command. Then check status
 * regularly. Only when the status becomes "idle" and @c counter has changed,
 * the index or mining has finished. If @c last_modified has not been changed,
 * it indicates a failure.
 *
 * @section request
 *
 * - @b collection* (@c String): Collection name.
 *
 * @section response
 *
 * - @b index (@c Object): Index status.
 *   - @b status (@c String): One of the following
 *     - @c running : the index process is alive and an index task is running.
 *     - @c idle    : the index process is alive but no index task is running.
 *     - @c stopped : the index process is stopped.
 *   - @b last_modified (@c UInt): Unix timestamp (seconds since 1970-01-01
 *     00:00:00 UTC) of the last modified time.
 *   - @b counter (@c UInt): A counter which is increased after each build
 * - @b mining (@c Object): Mining status. Same structure with @b index.
 */
void StatusController::index()
{
    // index
    Value& indexStatusResponse = response()[Keys::index];
    indexStatusResponse[Keys::status] = "stopped";
    indexStatusResponse[Keys::last_modified] = 0;
    indexStatusResponse[Keys::document_count] = 0;

    Status indexStatus;

    if (indexTaskService_->getIndexStatus(indexStatus))
    {
        indexStatusResponse[Keys::status] =
            indexStatus.running() ? "running" : "idle";
        if (indexStatus.running())
        {
            indexStatusResponse[Keys::progress] = indexStatus.progress_;
            indexStatusResponse[Keys::elapsed_time] = indexStatus.getElapsedTime();
            indexStatusResponse[Keys::left_time] = indexStatus.getLeftTime();
            indexStatusResponse[Keys::meta] = indexStatus.metaInfo_;
        }
        //indexStatusResponse[Keys::document_count] = indexStatus.numDocs_;
        indexStatusResponse[Keys::last_modified] = indexStatus.lastModified();
        indexStatusResponse[Keys::counter] = indexStatus.counter();
    }

    // mining
//     Value& miningStatusResponse = response()[Keys::mining];
//     miningStatusResponse[Keys::status] = "stopped";
//     miningStatusResponse[Keys::last_modified] = 0;
//
//     Status miningStatus;
//     if (BrokerAgentClient::instance().getMiningStatus(collectionName_, miningStatus))
//     {
//         miningStatusResponse[Keys::status] =
//             miningStatus.running() ? "running" : "idle";
//         miningStatusResponse[Keys::last_modified] = miningStatus.lastModified();
//     }
}

void StatusController::get_distribute_status()
{
    Value& statusResponse = response()[Keys::DistributeStatus];
    Value& memStatus = statusResponse[Keys::MemoryStatus];

    std::vector<std::string> keylist;
    std::vector<std::string> valuelist;
    std::vector<std::string> nodelist;
    DistributeTestSuit::getMemoryStateKeyList(keylist);
    DistributeTestSuit::getMemoryState(keylist, valuelist);
    NodeManagerBase::get()->getAllReplicaInfo(nodelist, true, true);
    for(size_t i = 0; i < keylist.size(); ++i)
    {
        memStatus[keylist[i]] = valuelist[i];
    }
    std::string nodeliststr;
    for(size_t i = 0; i < nodelist.size(); ++i)
    {
        nodeliststr += nodelist[i] + ", ";
    }
    memStatus["AliveNodeList"] = nodeliststr;
    memStatus["IsAnyWriteRunning"] = NodeManagerBase::get()->isAnyWriteRunningInReplicas()?"yes":"no";
}

} // namespace sf1r
