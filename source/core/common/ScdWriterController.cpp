#include "ScdWriterController.h"

using namespace sf1r;


ScdWriterController::ScdWriterController(const std::string& dir)
:dir_(dir), writer_(NULL), last_scd_type_(NOT_SCD), document_limit_(0), m_(0)
{
}

ScdWriterController::~ScdWriterController()
{
    Flush();
}

ScdWriter* ScdWriterController::GetWriter_(SCD_TYPE scd_type)
{
    if(writer_==NULL)
    {
        writer_ = new ScdWriter(dir_, scd_type);
        document_limit_ =  0;
    }
    else
    {
        if(scd_type!=last_scd_type_ || (m_>0 && document_limit_>=m_))
        {
            writer_->Close();
            delete writer_;
            writer_ = new ScdWriter(dir_, scd_type);
            document_limit_ =  0;
        }
    }
    last_scd_type_ = scd_type;
    return writer_;
}

bool ScdWriterController::Write(const SCDDoc& doc, SCD_TYPE scd_type)
{
    ScdWriter* writer = GetWriter_(scd_type);
    if(writer==NULL) return false;
    document_limit_++;
    return writer->Append(doc);
}

bool ScdWriterController::Write(const Document& doc, SCD_TYPE scd_type)
{
    ScdWriter* writer = GetWriter_(scd_type);
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
    last_scd_type_ = NOT_SCD;
}
