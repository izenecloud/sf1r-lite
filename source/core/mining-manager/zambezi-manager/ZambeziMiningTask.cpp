#include "ZambeziMiningTask.h"
#include <la-manager/AttrTokenizeWrapper.h>
#include <configuration-manager/ZambeziConfig.h>
#include <document-manager/DocumentManager.h>
#include <common/ResourceManager.h>
#include <glog/logging.h>
#include <fstream>

using namespace sf1r;

ZambeziMiningTask::ZambeziMiningTask(
    const ZambeziConfig& config,
    DocumentManager& documentManager,
    izenelib::ir::Zambezi::NewInvertedIndex& indexer)
    : config_(config)
    , documentManager_(documentManager)
    , indexer_(indexer)
    , startDocId_(0)
{
    if (config_.isDebug)
    {
        ofs_debug_.open((config_.indexFilePath + "debug").c_str(), ios::app);
    }
    
}

bool ZambeziMiningTask::buildDocument(docid_t docID, const Document& doc)
{
    std::string propValue;
    std::vector<std::string> propNameList;
    std::vector<std::string> propValueList;
    propNameList.push_back("Title");
    propNameList.push_back("Attribute");
    propNameList.push_back("Category");
    propNameList.push_back("OriginalCategory");
    propNameList.push_back("source");

    for (std::vector<std::string>::iterator i = propNameList.begin(); i != propNameList.end(); ++i)
    {
        doc.getProperty(*i, propValue);
        propValueList.push_back(propValue);
    }

    std::vector<std::pair<std::string, double> > tokenScoreList;
    tokenScoreList = AttrTokenizeWrapper::get()->attr_tokenize_index(propValueList[0]
                                                                , propValueList[1]
                                                                , propValueList[2]
                                                                , propValueList[3]
                                                                , propValueList[4]);
    std::vector<std::string> tokenList;
    std::vector<uint32_t> scoreList;

    for (std::vector<std::pair<std::string, double> >::const_iterator it =
             tokenScoreList.begin(); it != tokenScoreList.end(); ++it)
    {
        tokenList.push_back(it->first);
        scoreList.push_back(uint32_t(it->second));
    }

    if (config_.isDebug)
    {
        ofs_debug_ << docID << '\t' ;
        for (unsigned int i = 0; i < tokenList.size(); ++i)
        {
            ofs_debug_ << tokenList[i] << " " << scoreList[i] << " ; ";
        }
        ofs_debug_ << std::endl;
    }

    indexer_.insertDoc(docID, tokenList, scoreList);
    return true;
}

bool ZambeziMiningTask::preProcess(int64_t timestamp)
{
    startDocId_ = indexer_.totalDocNum() + 1;
    const docid_t endDocId = documentManager_.getMaxDocId();

    LOG(INFO) << "zambezi mining task"
              << ", start docid: " << startDocId_
              << ", end docid: " << endDocId;

    return startDocId_ <= endDocId;
}

bool ZambeziMiningTask::postProcess()
{
    indexer_.flush();

    std::ofstream ofs(config_.indexFilePath.c_str(), std::ios_base::binary);
    if (! ofs)
    {
        LOG(ERROR) << "failed opening file " << config_.indexFilePath;
        return false;
    }

    try
    {
        indexer_.save(ofs);
    }
    catch (const std::exception& e)
    {
        LOG(ERROR) << "exception in writing file: " << e.what()
                   << ", path: " << config_.indexFilePath;
        return false;
    }

    return true;
}
