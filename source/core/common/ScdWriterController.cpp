#include "ScdWriterController.h"

using namespace sf1r;


ScdWriterController::ScdWriterController(const std::string& dir)
:dir_(dir), writer_(NULL), last_op_(0), document_limit_(0), m_(0)
{
}

ScdWriterController::~ScdWriterController()
{
    Flush();
}

ScdWriter* ScdWriterController::GetWriter_(int op)
{
    if(writer_==NULL)
    {
        writer_ = new ScdWriter(dir_, op);
        document_limit_ =  0;
    }
    else
    {
        if(op!=last_op_ || (m_>0 && document_limit_>=m_))
        {
            writer_->Close();
            delete writer_;
            writer_ = new ScdWriter(dir_, op);
            document_limit_ =  0;
        }
    }
    last_op_ = op;
    return writer_;
}

bool ScdWriterController::Write(const SCDDoc& doc, int op)
{
    ScdWriter* writer = GetWriter_(op);
    if(writer==NULL) return false;
    document_limit_++;
    return writer->Append(doc);
}

bool ScdWriterController::Write(const Document& doc, int op)
{
    ScdWriter* writer = GetWriter_(op);
    if(writer==NULL) return false;
    document_limit_++;
    return writer->Append(doc);
}

void ScdWriterController::Flush()
{
    if(writer_!=NULL)
    {
        writer_->Close();
        delete writer_;
        writer_ = NULL;
    }
    last_op_ = 0;
}
