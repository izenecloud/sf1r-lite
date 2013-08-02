#include "MultiThreadMiningTaskBuilder.h"
#include "MiningTask.h"
#include <document-manager/DocumentManager.h>
#include <glog/logging.h>
#include <boost/thread/thread.hpp>
#include <boost/bind.hpp>

using namespace sf1r;

MultiThreadMiningTaskBuilder::MultiThreadMiningTaskBuilder(
    boost::shared_ptr<DocumentManager>& documentManager,
    std::size_t threadNum)
    : documentManager_(documentManager)
    , threadNum_(threadNum)
{
}

MultiThreadMiningTaskBuilder::~MultiThreadMiningTaskBuilder()
{
    for (std::vector<MiningTask*>::iterator it = taskList_.begin();
         it != taskList_.end(); ++it)
    {
        delete *it;
    }
}

void MultiThreadMiningTaskBuilder::addTask(MiningTask* task)
{
    if (task)
    {
        taskList_.push_back(task);
    }
    else
    {
        LOG(WARNING) << "failed to add a NULL mining task.";
    }
}

bool MultiThreadMiningTaskBuilder::buildCollection()
{
    docid_t endDocId = documentManager_->getMaxDocId();
    docid_t startDocId = endDocId;
    int buildNum = 0;

    taskFlag_.assign(taskList_.size(), false);

    for (std::size_t i = 0; i < taskList_.size(); ++i)
    {
        if (taskList_[i]->preProcess())
        {
            taskFlag_[i] = true;
            ++buildNum;
            startDocId = std::min(startDocId, taskList_[i]->getLastDocId());
        }
    }

    if (buildNum == 0)
    {
        LOG(INFO) << "No mining task needs to build.";
        return true;
    }

    LOG(INFO) << "start to build " << buildNum << " mining tasks in "
              << threadNum_ << " threads...";

    boost::thread_group threads;
    for (std::size_t threadId = 0; threadId < threadNum_; ++threadId)
    {
        threads.create_thread(
            boost::bind(&MultiThreadMiningTaskBuilder::buildDocs_, this,
                        startDocId, endDocId, threadId));
    }
    threads.join_all();

    std::cout << "\rbuilding doc id: " << endDocId << "\t" << std::endl;
    LOG(INFO) << "mining tasks are finished";

    for (std::size_t i = 0; i < taskList_.size(); ++i)
    {
        if (taskFlag_[i])
        {
            taskList_[i]->postProcess();
        }
    }

    return true;
}

void MultiThreadMiningTaskBuilder::buildDocs_(
    docid_t startDocId,
    docid_t endDocId,
    std::size_t threadId)
{
    for (docid_t docId = startDocId; docId <= endDocId; ++docId)
    {
        if (docId % threadNum_ != threadId)
            continue;

        if (docId % 10000 == 0)
        {
            std::cout << "\rbuilding doc id: " << docId << "\t" << std::flush;
        }

        Document doc;
        if (documentManager_->getDocument(docId, doc))
        {
            documentManager_->getRTypePropertiesForDocument(docId, doc);
        }

        for (std::size_t i = 0; i < taskList_.size(); ++i)
        {
            if (taskFlag_[i] && docId >= taskList_[i]->getLastDocId())
            {
                taskList_[i]->buildDocument(docId, doc);
            }
        }
    }
}
