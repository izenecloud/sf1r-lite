#include "CategoryClassifyMiningTask.h"
#include "CategoryClassifyTable.h"
#include <document-manager/DocumentManager.h>
#include <la-manager/KNlpWrapper.h>
#include <util/ustring/UString.h>
#include <glog/logging.h>
#include <boost/filesystem/path.hpp>

using namespace sf1r;
namespace bfs = boost::filesystem;

namespace
{
const std::string kDebugFileName = "debug.txt";

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
    const std::string& categoryPropName,
    bool isDebug)
    : documentManager_(documentManager)
    , classifyTable_(classifyTable)
    , categoryPropName_(categoryPropName)
    , startDocId_(0)
    , isDebug_(isDebug)
{
    if (isDebug_)
    {
        const bfs::path dirPath(classifyTable.dirPath());
        const bfs::path debugPath(dirPath / kDebugFileName);

        debugStream_.open(debugPath.string().c_str(), std::ofstream::app);
    }
}

bool CategoryClassifyMiningTask::buildDocument(docid_t docID, const Document& doc)
{
    std::string title;
    getDocPropValue(doc, classifyTable_.propName(), title);

    if (title.empty())
        return true;

    std::string classifyCategory;
    std::string category;
    if (!categoryPropName_.empty())
    {
        getDocPropValue(doc, categoryPropName_, category);
        classifyByCategory_(category, classifyCategory);
    }

    if (classifyCategory.empty())
    {
        classifyByTitle_(title, classifyCategory);
    }

    classifyTable_.setCategory(docID, classifyCategory);

    if (isDebug_)
    {
        debugStream_ << docID << " [" << classifyCategory << "] "
                     << title << std::endl;
    }

    return true;
}

void CategoryClassifyMiningTask::classifyByCategory_(
    const std::string& category,
    std::string& classifyCategory)
{
    if (category.find("图书音像") != std::string::npos)
    {
        classifyCategory = "R>文娱>书籍杂志";
    }
}

void CategoryClassifyMiningTask::classifyByTitle_(
    const std::string& title,
    std::string& classifyCategory)
{
    KNlpWrapper* knlpWrapper = KNlpWrapper::get();
    KNlpWrapper::string_t titleKStr(title);
    KNlpWrapper::token_score_list_t tokenScores;
    knlpWrapper->fmmTokenize(titleKStr, tokenScores);

    KNlpWrapper::string_t classifyKStr = knlpWrapper->classifyToBestCategory(tokenScores);
    classifyCategory = classifyKStr.get_bytes("utf-8");
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

    if (isDebug_)
    {
        debugStream_.flush();
    }

    return true;
}
