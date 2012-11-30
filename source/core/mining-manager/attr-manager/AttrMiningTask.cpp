#include "AttrMiningTask.h"
#include "../util/split_ustr.h"
#include "../util/FSUtil.hpp"
#include "../MiningException.hpp"
#include <document-manager/DocumentManager.h>
#include "AttrManager.h"
#include <glog/logging.h>
#include <iostream>

NS_FACETED_BEGIN
AttrMiningTask::AttrMiningTask(DocumentManager& documentManager
		, AttrManager& attrManager
		, std::string dirPath)
	:documentManager_(documentManager)
	, attrManager_(attrManager)
	, dirPath_(dirPath)
{

}

AttrMiningTask::~AttrMiningTask()
{

}

bool AttrMiningTask::processCollection_forTest()
{
    preProcess();
    docid_t MaxDocid = documentManager_.getMaxDocId();
    const docid_t startDocId = attrManager_.getAttrTable().docIdNum();
    Document doc;
    
    for (uint32_t docid = startDocId; docid <= MaxDocid; ++docid)
    {
        if (docid % 10000 == 0)
        {
            std::cout << "\rinserting doc id: " << docid << "\t" << std::flush;
        }
        documentManager_.getDocument(docid, doc);
        buildDocment(docid, doc);
    }
    postProcess();
    return true;
}

void AttrMiningTask::preProcess()
{
    try
    {
        FSUtil::createDir(dirPath_);
    }
    catch(FileOperationException& e)
    {
        LOG(ERROR) << "exception in FSUtil::createDir: " << e.what();
        return;
    }
}

docid_t AttrMiningTask::getLastDocId()
{
	return attrManager_.getAttrTable().docIdNum();
}

void AttrMiningTask::postProcess()
{
    const char* propName = attrManager_.getAttrTable().propName();  

    if (!attrManager_.getAttrTable().flush())
    {
        LOG(ERROR) << "AttrTable::flush() failed, property name: " << propName;
    }

    LOG(INFO) << "finished building attr index data";
}


bool AttrMiningTask::buildDocment(docid_t docID, Document& doc)
{
    if (doc.getId() == 0)
        return true;
    const char* propName = attrManager_.getAttrTable().propName();
    attrManager_.buildDocp_(docID, propName);
    return true;
}

NS_FACETED_END