#include "ZambeziMiningTask.h"
#include <document-manager/DocumentManager.h>
#include <common/ResourceManager.h>
#include <glog/logging.h>
#include <fstream>

using namespace sf1r;

ZambeziMiningTask::ZambeziMiningTask(
    DocumentManager& documentManager,
    izenelib::ir::Zambezi::InvertedIndex& indexer,
    const std::string& indexPropName,
    const std::string& indexFilePath)
    : documentManager_(documentManager)
    , indexer_(indexer)
    , indexPropName_(indexPropName)
    , indexFilePath_(indexFilePath)
    , startDocId_(0)
{
}

bool ZambeziMiningTask::buildDocument(docid_t docID, const Document& doc)
{
    std::string propValue;
    doc.getProperty(indexPropName_, propValue);

    KNlpWrapper::token_score_list_t tokenScores;
    KNlpWrapper::string_t kstr(propValue);
    boost::shared_ptr<KNlpWrapper> knlpWrapper = KNlpResourceManager::getResource();
    knlpWrapper->fmmTokenize(kstr, tokenScores);

    std::vector<std::string> tokenList;
    for (KNlpWrapper::token_score_list_t::const_iterator it =
             tokenScores.begin(); it != tokenScores.end(); ++it)
    {
        tokenList.push_back(it->first.get_bytes("utf-8"));
    }

    indexer_.insertDoc(docID, tokenList);
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

    std::ofstream ofs(indexFilePath_.c_str(), std::ios_base::binary);
    if (! ofs)
    {
        LOG(ERROR) << "failed opening file " << indexFilePath_;
        return false;
    }

    try
    {
        indexer_.save(ofs);
    }
    catch (const std::exception& e)
    {
        LOG(ERROR) << "exception in writing file: " << e.what()
                   << ", path: " << indexFilePath_;
        return false;
    }

    return true;
}
