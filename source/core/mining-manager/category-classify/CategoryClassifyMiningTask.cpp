#include "CategoryClassifyMiningTask.h"
#include "CategoryClassifyTable.h"
#include <document-manager/DocumentManager.h>
#include <la-manager/KNlpWrapper.h>
#include <knlp/doc_naive_bayes.h>
#include <util/ustring/UString.h>
#include <glog/logging.h>
#include <boost/filesystem/path.hpp>

using namespace sf1r;
namespace bfs = boost::filesystem;

namespace
{
const std::string kOriginalCategoryPropName("OriginalCategory");
const std::string kSourcePropName("Source");
const std::string kClassifyCategoryValueBook("R>文娱>书籍杂志");

void getDocPropValue(
    const Document& doc,
    const std::string& propName,
    std::string& propValue)
{
    izenelib::util::UString ustr;
    doc.getProperty(propName, ustr);
    ustr.convertString(propValue, izenelib::util::UString::UTF_8);
}

}

CategoryClassifyMiningTask::CategoryClassifyMiningTask(
    DocumentManager& documentManager,
    CategoryClassifyTable& classifyTable,
    const std::string& targetCategoryPropName)
    : documentManager_(documentManager)
    , classifyTable_(classifyTable)
    , targetCategoryPropName_(targetCategoryPropName)
    , startDocId_(0)
{
}

bool CategoryClassifyMiningTask::buildDocument(docid_t docID, const Document& doc)
{
    std::string title;
    getDocPropValue(doc, classifyTable_.propName(), title);

    if (title.empty())
        return true;

    std::string classifyCategory;
    std::string targetCategory;
    bool isRule = true;

    if (ruleByTargetCategory_(doc, targetCategory, classifyCategory) ||
        ruleByOriginalCategory_(doc, classifyCategory) ||
        ruleBySource_(doc, targetCategory, classifyCategory) ||
        classifyByTitle_(title, classifyCategory, isRule))
    {
        classifyTable_.setCategory(docID, classifyCategory, isRule);
    }

    return true;
}

bool CategoryClassifyMiningTask::ruleByTargetCategory_(
    const Document& doc,
    std::string& targetCategory,
    std::string& classifyCategory)
{
    if (!targetCategoryPropName_.empty())
    {
        getDocPropValue(doc, targetCategoryPropName_, targetCategory);
        if (targetCategory.find("图书音像") != std::string::npos)
        {
            classifyCategory = kClassifyCategoryValueBook;
            return true;
        }
    }
    return false;
}

bool CategoryClassifyMiningTask::ruleByOriginalCategory_(
    const Document& doc,
    std::string& classifyCategory)
{
    std::string originalCategory;
    getDocPropValue(doc, kOriginalCategoryPropName, originalCategory);

    if (!originalCategory.empty())
    {
        KNlpWrapper* knlpWrapper = KNlpWrapper::get();
        classifyCategory = knlpWrapper->mapFromOriginalCategory(originalCategory);
    }

    return !classifyCategory.empty();
}

bool CategoryClassifyMiningTask::ruleBySource_(
    const Document& doc,
    const std::string& targetCategory,
    std::string& classifyCategory)
{
    if (!targetCategory.empty())
        return false;

    std::string source;
    getDocPropValue(doc, kSourcePropName, source);

    if (source == "文轩网官网")
    {
        classifyCategory = kClassifyCategoryValueBook;
        return true;
    }

    return false;
}

bool CategoryClassifyMiningTask::classifyByTitle_(
    const std::string& title,
    std::string& classifyCategory,
    bool& isRule)
{
    try
    {
        KNlpWrapper* knlpWrapper = KNlpWrapper::get();
        std::string cleanTitle = knlpWrapper->cleanGarbage(title);
        KNlpWrapper::string_t titleKStr(cleanTitle);

        KNlpWrapper::token_score_list_t tokenScores;
        knlpWrapper->fmmTokenize(titleKStr, tokenScores);

        KNlpWrapper::string_t classifyKStr = knlpWrapper->classifyToBestCategory(tokenScores);
        classifyCategory = classifyKStr.get_bytes("utf-8");
        isRule = false;
        return true;
    }
    catch(std::exception& ex)
    {
        //LOG(ERROR) << "exception: " << ex.what() << title;
        return false;
    }
}

bool CategoryClassifyMiningTask::preProcess()
{
    startDocId_ = classifyTable_.docIdNum();
    const docid_t endDocId = documentManager_.getMaxDocId();

    LOG(INFO) << "category classify mining task"
              << ", start docid: " << startDocId_
              << ", end docid: " << endDocId;

    if (startDocId_ > endDocId)
        return false;

    classifyTable_.resize(endDocId + 1);
    return true;
}

bool CategoryClassifyMiningTask::postProcess()
{
    if (!classifyTable_.flush())
    {
        LOG(ERROR) << "failed in CategoryClassifyTable::flush()";
        return false;
    }

    return true;
}
