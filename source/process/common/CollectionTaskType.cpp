#include "CollectionTaskType.h"

#include <controllers/CollectionHandler.h>
#include <common/CollectionManager.h>
#include <common/XmlConfigParser.h>

#include <bundles/index/IndexTaskService.h>

namespace sf1r
{

void RebuildTask::doTask()
{
    std::cout << "start RebuildTask " << collectionName_ << std::endl;
    if (isRunning_)
        return;

    isRunning_ = true;

    // start a new collection for rebuilding
    std::string newCollectionName = collectionName_ + "-rebuild";
    std::string newConfigFile = SF1Config::get()->getCollectionConfigFile(newCollectionName);
    std::cout << newConfigFile << std::endl;
    CollectionHandler* newCollHandler = CollectionManager::get()->startCollection(collectionName_, newConfigFile, true);
    if (!newCollHandler)
    {
        return;
    }
    newCollHandler->indexTaskService_->optimizeIndexIdSpace();

    // stop current collection
    CollectionManager::get()->stopCollection(collectionName_);

    // switch data from new to current

    // start current collection
    std::string configFile = SF1Config::get()->getCollectionConfigFile(collectionName_);
    std::cout << configFile << std::endl;
    CollectionManager::get()->startCollection(collectionName_, configFile);

    std::cout << "end RebuildTask " << collectionName_ << std::endl;
    isRunning_ = false;
}

}
