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
    bool taskFlag[20];
    for (int i = 0; i < 20; ++i)
    {
        taskFlag[i] = false;
    }
    int lable = 0;
    for (std::vector<MiningTask*>::iterator iter = taskList_.begin(); iter != taskList_.end(); ++iter)
    {
        {
            taskFlag[lable] = true;
            if(!(*iter)->preProcess())
            {
                taskFlag[lable] = false;
            }
        }
        lable++;
    }
    for (std::vector<MiningTask*>::iterator iter = taskList_.begin(); iter != taskList_.end(); ++iter)
    {
        if (min_last_docid > (*iter)->getLastDocId())
        {
            min_last_docid = (*iter)->getLastDocId();
        }
    }

    LOG(INFO) << "begin build Collection...." << endl;
    for (uint32_t docid = min_last_docid; docid <= MaxDocid; ++docid)
    {
        Document doc;
        lable = 0;
        if (docid % 10000 == 0)
        {
            std::cout << "\rinserting doc id: " << docid << "\t" << std::flush;
        }

        if (document_manager_->getDocument(docid, doc))
        {
            document_manager_->getRTypePropertiesForDocument(docid, doc);
        }

        for (std::vector<MiningTask*>::iterator iter = taskList_.begin(); iter != taskList_.end(); ++iter)
        {
            if ( taskFlag[lable])
            {
                docid_t currentDocid = (*iter)->getLastDocId();
                if (docid < currentDocid)
                continue;
                (*iter)->buildDocment(docid, doc);
            }
            lable++;
        }
    }
    LOG(INFO) << "build Collection end" << endl;
    lable = 0;
    for (std::vector<MiningTask*>::iterator iter = taskList_.begin(); iter != taskList_.end(); ++iter)
    {
        if ( taskFlag[lable])
        {
            (*iter)->postProcess();
        }
        lable++;
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
        LOG(INFO) << "ONE MiningTask IS ERORR" << endl;
    }
}
}
