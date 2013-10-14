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
        izenelib::ir::Zambezi::AttrScoreInvertedIndex& indexer)
    : config_(config)
    , documentManager_(documentManager)
    , indexer_(indexer)
    , startDocId_(0)
{
}

bool ZambeziMiningTask::buildDocument(docid_t docID, const Document& doc)
{
    std::vector<std::string> propNameList;
    std::vector<std::string> propValueList;
    propNameList.push_back("Title");
    propNameList.push_back("Attribute");
    propNameList.push_back("Category");
    propNameList.push_back("OriginalCategory");
    propNameList.push_back("Source");

    for (std::vector<std::string>::iterator i = propNameList.begin(); i != propNameList.end(); ++i)
    {
        std::string propValue;
        doc.getProperty(*i, propValue);
        if (*i == "Attribute")
        {
            std::string brand;
            doc.getProperty("Brand", brand);
            if (!brand.empty())
            {
                propValue += (",brand:" + brand);
            }
        }
        propValueList.push_back(propValue);
    }

    std::vector<std::pair<std::string, double> > tokenScoreList;
    AttrTokenizeWrapper::get()->attr_tokenize_index(propValueList[0]
                                                                , propValueList[1]
                                                                , propValueList[2]
                                                                , propValueList[3]
                                                                , propValueList[4]
                                                                , tokenScoreList);
    std::vector<std::string> tokenList;
    std::vector<uint32_t> scoreList;

    for (std::vector<std::pair<std::string, double> >::const_iterator it =
             tokenScoreList.begin(); it != tokenScoreList.end(); ++it)
    {
        tokenList.push_back(it->first);
        scoreList.push_back(uint32_t(it->second));
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
