#include "ProductForwardMiningTask.h"
#include "../suffix-match-manager/ProductTokenizer.h"
#include <fstream>
#include <boost/filesystem.hpp>
#include <glog/logging.h>
#include <icma/icma.h>
#include <document-manager/DocumentManager.h>

namespace sf1r
{

ProductForwardMiningTask::ProductForwardMiningTask(boost::shared_ptr<DocumentManager>& document_manager
                    , ProductTokenizer* tokenizer
                    , ProductForwardManager* forward)
    : document_manager_(document_manager)
    , forward_index_(forward)
    , tokenizer_(tokenizer)
{
}

ProductForwardMiningTask::~ProductForwardMiningTask()
{
}

bool ProductForwardMiningTask::buildDocument(docid_t docID, const Document& doc)
{
    bool failed = (doc.getId() == 0);
    std::string src;

    if (!failed)
    {
        const std::string pname("Title");
        doc.getString(pname, src);
        std::string res;
        tokenizer_->GetFeatureTerms(src, res);
        tmp_index_.push_back(res);
    }
    else//插入空串
        tmp_index_.push_back(src);

    return true;
}



docid_t ProductForwardMiningTask::getLastDocId()
{
    return forward_index_->getLastDocId() + 1;
}

bool ProductForwardMiningTask::preProcess(int64_t timestamp)
{
    if (forward_index_->getLastDocId() == 0)
    {
        forward_index_->clear();
    }
    if (forward_index_->getLastDocId() + 1 > document_manager_->getMaxDocId())
        return false;
    forward_index_->copy(tmp_index_);
    tmp_index_.reserve(document_manager_->getMaxDocId() + 1);
    return true;
}

bool ProductForwardMiningTask::postProcess()
{
    LOG (INFO) << "Save Forward Index ......" ;
    forward_index_->insert(tmp_index_);
    LOG(INFO)<<"insert ok";
    std::vector<std::string>().swap(tmp_index_);
    LOG(INFO)<<"clear ok";
    forward_index_->save(document_manager_->getMaxDocId());
    LOG(INFO)<<"save ok";
    return true;
}

}
