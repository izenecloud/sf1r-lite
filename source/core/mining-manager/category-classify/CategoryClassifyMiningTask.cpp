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
}

CategoryClassifyMiningTask::CategoryClassifyMiningTask(
    DocumentManager& documentManager,
    CategoryClassifyTable& categoryTable,
    bool isDebug)
    : documentManager_(documentManager)
    , categoryTable_(categoryTable)
    , startDocId_(0)
    , isDebug_(isDebug)
{
    if (isDebug_)
    {
        const bfs::path dirPath(categoryTable.dirPath());
        const bfs::path debugPath(dirPath / kDebugFileName);

        debugStream_.open(debugPath.string().c_str(), std::ofstream::app);
    }
}

bool CategoryClassifyMiningTask::buildDocument(docid_t docID, const Document& doc)
{
    izenelib::util::UString propValueUStr;
    doc.getProperty(categoryTable_.propName(), propValueUStr);

    if (propValueUStr.empty())
        return true;

    std::string propValue;
    propValueUStr.convertString(propValue, izenelib::util::UString::UTF_8);

    KNlpWrapper* knlpWrapper = KNlpWrapper::get();
    KNlpWrapper::string_t propValueKStr(propValue);
    KNlpWrapper::token_score_list_t tokenScores;
    knlpWrapper->fmmTokenize(propValueKStr, tokenScores);

    KNlpWrapper::string_t categoryKStr = knlpWrapper->classifyToBestCategory(tokenScores);
    std::string category = categoryKStr.get_bytes("utf-8");
    categoryTable_.setCategory(docID, category);

    if (isDebug_)
    {
        debugStream_ << docID << " [" << category << "] "
                     << propValue << std::endl;
    }

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

    if (isDebug_)
    {
        debugStream_.flush();
    }

    return true;
}
