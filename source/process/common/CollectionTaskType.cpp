#include "CollectionTaskType.h"

namespace sf1r
{

void RebuildTask::doTask()
{
    std::cout << "start RebuildTask " << collectionName_ << std::endl;

    if (!CollectionTaskType::checkStart())
        return;
    else
        isRunning_ = true;

    // start new collection

    // stop current collection

    // switch data from new to current

    // start current collection

    isRunning_ = false;
}

}
