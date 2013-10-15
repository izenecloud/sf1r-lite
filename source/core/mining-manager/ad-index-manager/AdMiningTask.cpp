/*
 *  AdMiningTask.cpp
 */

#include "AdMiningTask.h"

namespace sf1r
{

AdMiningTask::AdMiningTask(
        const std::string& path,
        boost::shared_ptr<DocumentManager>& dm)
    :indexPath_(path), documentManager_(dm)
{
    adIndex_.reset(new AdIndexType());
}

AdMiningTask::~AdMiningTask()
{
}

bool AdMiningTask::buildDocument(docid_t docID, const Document& doc)
{
    std::string sDnf;
    doc.getProperty("DNF", sDnf);
    DNF dnf;
    sf1r::DNFParser::parseDNF(sDnf, dnf);
    adIndex_->addDNF(docID, dnf);
    return true;
}

bool AdMiningTask::preProcess(int64_t timestamp)
{
    startDocId_ = adIndex_->totalNumDNF()+1;
    const docid_t endDocId = documentManager_->getMaxDocId();

    LOG(INFO) << "AdIndex mining task"
              << ", start docid: " << startDocId_
              << ", end docid: " << endDocId <<endl;;
    return startDocId_ <= endDocId;
}

bool AdMiningTask::postProcess()
{
    std::ofstream ofs(indexPath_.c_str(), std::ios_base::binary);
    if(!ofs)
    {
        LOG(INFO) << "failed openning file " << indexPath_<<endl;
        return false;
    }

    try
    {
        adIndex_->save(ofs);
    }
    catch(const std::exception& e)
    {
        LOG(ERROR) << "exception in writing file "<<e.what()
                   << ", path: " <<indexPath_<<endl;
        return false;
    }
    return true;
}

void AdMiningTask::save()
{
    postProcess();
}

bool AdMiningTask::load()
{
    std::ifstream ifs(indexPath_.c_str(), std::ios_base::binary);
    if(!ifs)
        return false;
    try
    {
        adIndex_->load(ifs);
    }
    catch(const std::exception& e)
    {
        LOG(ERROR) << "exception in read file: " << e.what()
                   << ", path: "<< indexPath_<<endl;
        return false;
    }
    return true;
}

} //namespace sf1r
