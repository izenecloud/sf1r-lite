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
    if (!sf1r::DNFParser::parseDNF(sDnf, dnf))
        return false;
    std::string w,h,t;
    doc.getProperty("Width", w);
    doc.getProperty("Height", h);
    doc.getProperty("CreativeType", t);
    Assignment a1,a2,a3;
    a1.belongsTo = true;
    a2.belongsTo = true;
    a3.belongsTo = true;
    a1.attribute = "Width";
    a2.attribute = "Height";
    a3.attribute = "CreativeType";
    a1.values.push_back(w);
    a2.values.push_back(h);
    a3.values.push_back(t);
    dnf.conjunctions[0].assignments.push_back(a1);
    dnf.conjunctions[0].assignments.push_back(a2);
    dnf.conjunctions[0].assignments.push_back(a3);
    incrementalAdIndex_->addDNF(docID, dnf);
    return true;
}

bool AdMiningTask::preProcess(int64_t timestamp)
{
    incrementalAdIndex_.reset(new AdIndexType);

    LOG(INFO) << "totalDNFNUM: "<<adIndex_->totalNumDNF()<<endl;
    if(adIndex_->totalNumDNF() > 0)
    {
        std::ifstream ifs(indexPath_.c_str(), std::ios_base::binary);
        if(!ifs)
            return false;
        try
        {
            incrementalAdIndex_->load_binary(ifs);
        }
        catch(const std::exception& e)
        {
            LOG(ERROR) << "exception in read file: " << e.what()
                       << ", path: " << indexPath_<<endl;
            return false;
        }
    }

    const docid_t endDocId = documentManager_->getMaxDocId();
    startDocId_ = incrementalAdIndex_->totalNumDNF()+1;

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
        incrementalAdIndex_->save_binary(ofs);
    }
    catch(const std::exception& e)
    {
        LOG(ERROR) << "exception in writing file "<<e.what()
                   << ", path: " <<indexPath_<<endl;
        return false;
    }

    writeLock lock(rwMutex_);
    adIndex_ = incrementalAdIndex_;

    incrementalAdIndex_.reset();
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
        writeLock lock(rwMutex_);
        adIndex_->load_binary(ifs);
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
