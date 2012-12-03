#include "MiningTaskBuilder.h"
#include <document-manager/DocumentManager.h>
#include <glog/logging.h>

namespace sf1r
{
	MiningTaskBuilder::MiningTaskBuilder(boost::shared_ptr<DocumentManager> document_manager)
		: document_manager_(document_manager)
	{

	}

	bool MiningTaskBuilder::buildCollection()
	{

		for (std::vector<MiningTask*>::iterator iter = taskList_.begin(); iter != taskList_.end(); ++iter)
		{
			(*iter)->preProcess();
		}

		docid_t MaxDocid = document_manager_->getMaxDocId();
		docid_t min_last_docid = MaxDocid;
		
		for (std::vector<MiningTask*>::iterator iter = taskList_.begin(); iter != taskList_.end(); ++iter)
		{
			if (min_last_docid > (*iter)->getLastDocId())
			{
				min_last_docid = (*iter)->getLastDocId();
			}
		}

		Document doc;
		LOG(INFO)<<"begin build Collection...."<<endl;
		for (uint32_t docid = min_last_docid; docid <= MaxDocid; ++docid)
		{
			if (docid % 10000 == 0)
        	{
            	std::cout << "\rinserting doc id: " << docid << "\t" << std::flush;
        	}

			document_manager_->getDocument(docid, doc);	
			for (std::vector<MiningTask*>::iterator iter = taskList_.begin(); iter != taskList_.end(); ++iter)
			{
				docid_t currentDocid = (*iter)->getLastDocId();
				if (docid < currentDocid)
					continue;
				(*iter)->buildDocment(docid, doc);
			}

			doc.setId(0);
		}
		LOG(INFO)<<"build Collection end"<<endl;

		for (std::vector<MiningTask*>::iterator iter = taskList_.begin(); iter != taskList_.end(); ++iter)
		{
			(*iter)->postProcess();
		}
		taskList_.clear();
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
			LOG(INFO)<<"ONE MiningTask IS ERORR"<<endl;
		}
	}
}