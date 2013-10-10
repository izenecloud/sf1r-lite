/*
 *  AdMiningTask.cpp
 */

#include "AdMiningTask.h"

namespace sf1r
{
AdMiningTask::AdMiningTask(
        const std::string& path,
        boost::shared_ptr<DocumentManager> dm)
    :indexPath_(path), documentManager_(dm)
{
    adIndex_.reset(new AdIndexType());
}
AdMiningTask::~AdMiningTask()
{
}

bool AdMiningTask::buildDocument(docid_t docID, const Document& doc)
{
    return true;
}
bool AdMiningTask::preProcess(int64_t timestamp)
{
    return true;
}
bool AdMiningTask::postProcess()
{
    return true;
}
void AdMiningTask::save()
{
}
bool AdMiningTask::load()
{
    return true;
}
} //namespace sf1r
