#include "scd_operation_processor.h"
#include <sstream>
#include <common/ScdWriter.h>
#include <boost/filesystem.hpp>
#include <node-manager/DistributedSynchroFactory.h>

using namespace sf1r;

ScdOperationProcessor::ScdOperationProcessor(const std::string& dir)
:dir_(dir), writer_(NULL), last_op_(0)
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
    std::cout<<"ScdOperationProcessor::Finish "<<dir_<<std::endl;

    SynchroProducerPtr syncProducer =
            DistributedSynchroFactory::makeProcuder(DistributedSynchroFactory::SYNCHRO_TYPE_PRODUCT_MANAGER);

    if (syncProducer->produce(dir_, boost::bind(&ScdOperationProcessor::AfterProcess_, this,_1)))
    {
        bool isConsumed = false;
        syncProducer->waitConsumers(isConsumed);
        return isConsumed;
    }

    return false;
}

void ScdOperationProcessor::AfterProcess_(bool is_succ)
{
    if(is_succ)
    {
        std::cout<<"Scd transmission and processed successfully!"<<std::endl;
        namespace bfs = boost::filesystem;
        static const bfs::directory_iterator kItrEnd;
        
        for (bfs::directory_iterator itr(dir_); itr != kItrEnd; ++itr)
        {
            bfs::remove_all(itr->path());
        }
    }
    else
    {
        std::cout<<"ERROR: Scd transmission and processed failed!"<<std::endl;
    }
}


