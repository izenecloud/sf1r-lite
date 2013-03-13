#include "MiningTaskBuilder.h"
#include <document-manager/DocumentManager.h>
#include <glog/logging.h>
#include <vector>

namespace sf1r
{
MiningTaskBuilder::MiningTaskBuilder(boost::shared_ptr<DocumentManager> document_manager)
    : document_manager_(document_manager)
{
}

MiningTaskBuilder::~MiningTaskBuilder()
{
    for (std::vector<MiningTask*>::iterator iter = taskList_.begin(); iter != taskList_.end(); ++iter)
    {
        delete (*iter);
    }
}

bool MiningTaskBuilder::buildCollection()
{
    docid_t MaxDocid = document_manager_->getMaxDocId();

    docid_t min_last_docid = MaxDocid;

    std::vector<bool> taskFlag(taskList_.size(), true);

    size_t false_count = 0;
    for (size_t i = 0; i < taskList_.size(); ++i)
    {
        if (!taskList_[i]->preProcess())
        {
            taskFlag[i] = false;
            ++false_count;
        }
    }

    if (false_count == taskList_.size()) 
    {
        LOG (INFO) << "No build Collection...";
        return true;
    }
    for (size_t i = 0; i < taskList_.size(); ++i)
    {
        min_last_docid = std::min(min_last_docid, taskList_[i]->getLastDocId());
    }

    LOG(INFO) << "begin build Collection....";
    for (uint32_t docid = min_last_docid; docid <= MaxDocid; ++docid)
    {
        Document doc;
        if (docid % 10000 == 0)
        {
            std::cout << "\rinserting doc id: " << docid << "\t" << std::flush;
        }

        if (document_manager_->getDocument(docid, doc))
        {
            document_manager_->getRTypePropertiesForDocument(docid, doc);
        }

        for (size_t i = 0; i < taskList_.size(); ++i)
        {
            if (taskFlag[i])
            {
                if (docid < taskList_[i]->getLastDocId()) continue;
                taskList_[i]->buildDocument(docid, doc);
            }
        }
    }

    LOG(INFO) << "build Collection end";
    for (size_t i = 0; i < taskList_.size(); ++i)
    {
        if (taskFlag[i])
        {
            taskList_[i]->postProcess();
        }
    }

    return true;
}

void MiningTaskBuilder::addTask(MiningTask* miningTask)
{
    if (miningTask)
    {
        taskList_.push_back(miningTask);
    }
    else
    {
        LOG(INFO) << "ONE MiningTask IS ERORR";
    }
}
}
