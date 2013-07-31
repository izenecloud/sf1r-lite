#include "TitleScoreMiningTask.h"
#include "../suffix-match-manager/ProductTokenizer.h"
#include <fstream>
#include <boost/filesystem.hpp>
#include <glog/logging.h>
#include <icma/icma.h>
#include <la-manager/KNlpWrapper.h>
#include <document-manager/DocumentManager.h>

namespace sf1r
{

TitleScoreMiningTask::TitleScoreMiningTask(boost::shared_ptr<DocumentManager>& document_manager
                    , ProductTokenizer* tokenizer
                    , TitleScoreList* titleScoreList)
    : document_manager_(document_manager)
    , titleScoreList_(titleScoreList)
    , tokenizer_(tokenizer)
{
}

TitleScoreMiningTask::~TitleScoreMiningTask()
{
}

bool TitleScoreMiningTask::buildDocument(docid_t docID, const Document& doc)
{
    bool failed = (doc.getId() == 0);
    if (!failed)
    {
        const std::string pname = titleScoreList_->propName();
        std::string pattern;
        doc.getString(pname, pattern);
        double queryScore = 0;
        tokenizer_->GetQuerySumScore(pattern, queryScore, docID);
        titleScoreList_->insert(docID, queryScore);
    }
    else
        titleScoreList_->insert(docID, 0);

    return true;
}

docid_t TitleScoreMiningTask::getLastDocId()
{
    return titleScoreList_->getLastDocId_() + 1;
}

bool TitleScoreMiningTask::preProcess()
{
    if (titleScoreList_->getLastDocId_() == 0)
    {
        titleScoreList_->clearDocumentScore();
    }

    if (titleScoreList_->getLastDocId_() < document_manager_->getMaxDocId())
    {
        titleScoreList_->resize(document_manager_->getMaxDocId() + 1);
        titleScoreList_->insert(0, 0);
    }

    return true;
}

bool TitleScoreMiningTask::postProcess()
{
    LOG (INFO) << "Save Document Scores ......" ;
    titleScoreList_->saveDocumentScore(document_manager_->getMaxDocId());
    return true;
}

}