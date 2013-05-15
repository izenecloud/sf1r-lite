#ifndef SF1V5_DOCUMENT_MANAGER_SCDDOCUMENT_H
#define SF1V5_DOCUMENT_MANAGER_SCDDOCUMENT_H
#include <common/type_defs.h>
#include <common/ScdParser.h>
#include "Document.h"

namespace sf1r
{

class ScdDocument : public Document
{
public:
    ScdDocument():Document(), type(NOT_SCD)
    {
    }
    ScdDocument(SCD_TYPE t):Document(), type(t)
    {
    }
    ScdDocument(const Document& d, SCD_TYPE t):Document(d), type(t)
    {
    }

    void merge(const ScdDocument& doc)
    {
        type=doc.type;
        if(type==NOT_SCD||type==DELETE_SCD) clear();
        else
        {
            Document::merge(doc);
        }
    }

public:
  SCD_TYPE type;
};

}

#endif

