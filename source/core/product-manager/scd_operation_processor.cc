#include "scd_operation_processor.h"
#include <sstream>
#include <common/ScdWriter.h>

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

void ScdOperationProcessor::ScdEnd()
{
}

void ScdOperationProcessor::Finish()
{
    if(writer_!=NULL)
    {
        writer_->Close();
        delete writer_;
        writer_ = NULL;
        last_op_ = 0;
        //notify zookeeper on dir_
    }
}

