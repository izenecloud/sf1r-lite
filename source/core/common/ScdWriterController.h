#ifndef SF1R_COMMON_SCDWRITERCONTROLLER_H_
#define SF1R_COMMON_SCDWRITERCONTROLLER_H_

#include "ScdWriter.h"
#include <document-manager/Document.h>
#include <boost/function.hpp>
#include <boost/variant.hpp>
namespace sf1r
{
    

class ScdWriterController
{

public:
    ScdWriterController(const std::string& dir);

    ~ScdWriterController();

    bool Write(const SCDDoc& doc, SCD_TYPE scd_type);
    bool Write(const Document& doc, SCD_TYPE scd_type);
    
    void Flush();

    void SetFlushLimit(int m) {m_ = m;}
    
    
private:
    
    ScdWriter* GetWriter_(SCD_TYPE scd_type);
    
    
    
private:

    std::string dir_;
    ScdWriter* writer_;
    SCD_TYPE last_scd_type_;
    int document_limit_;
    int m_;
};


}

#endif

