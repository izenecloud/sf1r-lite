#include "scd_operation_processor.h"
#include <sstream>
#include <glog/logging.h>
#include <common/ScdWriter.h>
#include <boost/filesystem.hpp>
#include <node-manager/synchro/SynchroFactory.h>
#include <node-manager/DistributeRequestHooker.h>

using namespace sf1r;

ScdOperationProcessor::ScdOperationProcessor(
        const std::string& syncId,
        const std::string& collectionName,
        const std::string& dir)
: collectionName_(collectionName)
, syncId_(syncId)
, dir_(dir)
, writer_(NULL)
, last_scd_type_(NOT_SCD)
{
}

ScdOperationProcessor::~ScdOperationProcessor()
{
}


void ScdOperationProcessor::Append(SCD_TYPE scd_type, const PMDocumentType& doc)
{
    if(last_scd_type_ != NOT_SCD && scd_type != last_scd_type_)
    {
        writer_->Close();
        delete writer_;
        writer_ = NULL;
    }
    if(writer_==NULL)
    {
        writer_ = new ScdWriter(dir_, scd_type);
    }
    writer_->Append(doc);
    last_scd_type_ = scd_type;
}

void ScdOperationProcessor::Clear()
{
    if(writer_!=NULL)
    {
        writer_->Close();
        delete writer_;
        writer_ = NULL;
    }
    last_scd_type_ = NOT_SCD;
    ClearScds_();
}

bool ScdOperationProcessor::Finish()
{
    if(writer_!=NULL)
    {
        writer_->Close();
        delete writer_;
        writer_ = NULL;
        last_scd_type_ = NOT_SCD;
        //notify zookeeper on dir_
    }
    LOG(INFO)<<"ScdOperationProcessor::Finish "<<dir_<<std::endl;

    if (!DistributeRequestHooker::get()->isRunningPrimary())
    {
        LOG(INFO) << "not primary no need send scd files.";
        return true;
    }

    SynchroData syncData;
    syncData.setValue(SynchroData::KEY_COLLECTION, collectionName_);
    syncData.setValue(SynchroData::KEY_DATA_TYPE, SynchroData::DATA_TYPE_SCD_INDEX);
    syncData.setValue(SynchroData::KEY_DATA_PATH, dir_);

    SynchroProducerPtr syncProducer = SynchroFactory::getProducer(syncId_);
    if (syncProducer->produce(syncData, boost::bind(&ScdOperationProcessor::AfterProcess_, this,_1)))
    {
        return syncProducer->wait();
    }
    return false;
}

void ScdOperationProcessor::ClearScds_()
{
    LOG(INFO)<<"Clearing Scds"<<std::endl;
   namespace bfs = boost::filesystem;
    static const bfs::directory_iterator kItrEnd;

    for (bfs::directory_iterator itr(dir_); itr != kItrEnd; ++itr)
    {
        if (itr->path().extension() == ".SCD")
            bfs::remove_all(itr->path());
    }
}

void ScdOperationProcessor::AfterProcess_(bool is_succ)
{
    if(is_succ)
    {
        LOG(INFO)<<"Scd transmission and processed successfully!"<<std::endl;
        ClearScds_();
    }
    else
    {
        LOG(ERROR)<<"ERROR: Scd transmission and processed failed!"<<std::endl;
    }
}
