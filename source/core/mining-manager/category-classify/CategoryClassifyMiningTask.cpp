#include "CategoryClassifyMiningTask.h"
#include "CategoryClassifyTable.h"
#include <document-manager/DocumentManager.h>
#include <la-manager/KNlpWrapper.h>
#include <util/ustring/UString.h>
#include <glog/logging.h>

using namespace sf1r;

CategoryClassifyMiningTask::CategoryClassifyMiningTask(
    DocumentManager& documentManager,
    CategoryClassifyTable& categoryTable)
    : documentManager_(documentManager)
    , categoryTable_(categoryTable)
    , startDocId_(0)
{
}

bool CategoryClassifyMiningTask::buildDocument(docid_t docID, const Document& doc)
{
    izenelib::util::UString propValue;
    doc.getProperty(categoryTable_.propName(), propValue);

    if (propValue.empty())
        return true;

    std::string utf8Str;
    propValue.convertString(utf8Str, izenelib::util::UString::UTF_8);

    KNlpWrapper* knlpWrapper = KNlpWrapper::get();
    KNlpWrapper::string_t kStr(utf8Str);
    KNlpWrapper::token_score_list_t tokenScores;
    knlpWrapper->fmmTokenize(kStr, tokenScores);

    KNlpWrapper::string_t category = knlpWrapper->classifyToBestCategory(tokenScores);
    categoryTable_.setCategory(docID, category.get_bytes("utf-8"));

    return true;
}

bool CategoryClassifyMiningTask::preProcess()
{
    startDocId_ = categoryTable_.docIdNum();
    const docid_t endDocId = documentManager_.getMaxDocId();

    LOG(INFO) << "category classify mining task"
              << ", start docid: " << startDocId_
              << ", end docid: " << endDocId;

    if (startDocId_ > endDocId)
        return false;

    categoryTable_.resize(endDocId + 1);
    return true;
}

bool CategoryClassifyMiningTask::postProcess()
{
    if (!categoryTable_.flush())
    {
        LOG(ERROR) << "failed in CategoryClassifyTable::flush()";
        return false;
    }
    return true;
}
