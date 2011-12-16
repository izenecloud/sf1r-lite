#include "scd_operation_processor.h"
#include <sstream>
#include <glog/logging.h>
#include <common/ScdWriter.h>
#include <boost/filesystem.hpp>
#include <node-manager/synchro/SynchroFactory.h>

using namespace sf1r;

ScdOperationProcessor::ScdOperationProcessor(const std::string& collectionName, const std::string& dir)
:collectionName_(collectionName), dir_(dir), writer_(NULL), last_op_(0)
{
}

ScdOperationProcessor::~ScdOperationProcessor()
{
}


void ScdOperationProcessor::Append(int op, const PMDocumentType& doc)
{
    if(last_op_>0 && op!=last_op_)
    {
        writer_->Close();
        delete writer_;
        writer_ = NULL;
    }
    if(writer_==NULL)
    {
        writer_ = new ScdWriter(dir_, op);
    }
    writer_->Append(doc);
    last_op_ = op;
}

void ScdOperationProcessor::Clear()
{
    if(writer_!=NULL)
    {
        writer_->Close();
        delete writer_;
        writer_ = NULL;
    }
    last_op_ = 0;
    ClearScds_();
}

bool ScdOperationProcessor::Finish()
{
    if(writer_!=NULL)
    {
        writer_->Close();
        delete writer_;
        writer_ = NULL;
        last_op_ = 0;
        //notify zookeeper on dir_
    }
    LOG(INFO)<<"ScdOperationProcessor::Finish "<<dir_<<std::endl;


    SynchroData syncData;
    syncData.setValue(SynchroData::KEY_COLLECTION, collectionName_);
    syncData.setValue(SynchroData::KEY_DATA_PATH, dir_);

    SynchroProducerPtr syncProducer = SynchroFactory::getProducer(collectionName_);
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
