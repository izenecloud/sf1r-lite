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

    bool Write(const SCDDoc& doc, int op);
    
    void Flush();
    
    
private:
    
    ScdWriter* GetWriter_(int op);
    
    
    
private:

    std::string dir_;
    ScdWriter* writer_;
    int last_op_;
    int document_limit_;
    static const int M = 500;
};


}

#endif

