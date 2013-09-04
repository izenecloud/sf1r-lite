#include "ZambeziMiningTask.h"
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
}

bool ZambeziMiningTask::buildDocument(docid_t docID, const Document& doc)
{
    std::string propValue;
    doc.getProperty(config_.indexPropName, propValue);

    KNlpWrapper::token_score_list_t tokenScores;
    KNlpWrapper::string_t kstr(propValue);
    boost::shared_ptr<KNlpWrapper> knlpWrapper = KNlpResourceManager::getResource();
    knlpWrapper->fmmTokenize(kstr, tokenScores);

    std::vector<std::string> tokenList;
    std::vector<uint32_t> scoreList;
    // TODO
    // currently each term is assigned with a default score,
    // it should be replaced when the calculation interface is available
    const uint32_t defaultScore = 1;

    for (KNlpWrapper::token_score_list_t::const_iterator it =
             tokenScores.begin(); it != tokenScores.end(); ++it)
    {
        tokenList.push_back(it->first.get_bytes("utf-8"));
        scoreList.push_back(defaultScore);
    }

    indexer_.insertDoc(docID, tokenList, scoreList);
    return true;
}

bool ZambeziMiningTask::preProcess()
{
    // TODO
    // startDocId_ = indexer_.maxDocId() + 1;
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
