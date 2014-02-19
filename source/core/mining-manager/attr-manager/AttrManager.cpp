#include "AttrManager.h"
#include "../util/split_ustr.h"
#include "../util/FSUtil.hpp"
#include "../MiningException.hpp"
#include <document-manager/DocumentManager.h>

#include <glog/logging.h>
#include <iostream>

using namespace sf1r::faceted;

AttrManager::AttrManager(
        const AttrConfig& attrConfig,
        const std::string& dirPath,
        DocumentManager& documentManager)
    : attrConfig_(attrConfig)
    , dirPath_(dirPath)
    , documentManager_(documentManager)
    , attrMiningTask_(NULL)
{
}

bool AttrManager::open()
{
    LOG(INFO) << "Start loading attr directory: " << dirPath_;

    if (!attrTable_.open(dirPath_, attrConfig_.propName))
    {
        LOG(ERROR) << "AttrTable::open() failed, property name: " << attrConfig_.propName;
        return false;
    }

    LOG(INFO) << "End attr loading";
    attrMiningTask_ = new AttrMiningTask(documentManager_, attrTable_, dirPath_, attrConfig_);

    if(attrMiningTask_ == NULL)
    {
        LOG(INFO)<<"Build AttrMiningTask ERROR"<<endl;
        return false;
    }

    try
    {
        FSUtil::createDir(dirPath_);//xxx
    }
    catch(FileOperationException& e)
    {
        LOG(ERROR) << "exception in FSUtil::createDir: " << e.what();
        return false;
    }
    return true;
}
