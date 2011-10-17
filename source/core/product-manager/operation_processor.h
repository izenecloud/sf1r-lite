#ifndef SF1R_PRODUCTMANAGER_OPERATIONPROCESSOR_H
#define SF1R_PRODUCTMANAGER_OPERATIONPROCESSOR_H

#include <common/type_defs.h>

#include <boost/operators.hpp>

#include "pm_def.h"
#include "pm_types.h"


namespace sf1r
{

class OperationProcessor
{
public:
    enum {INSERT = 1, UPDATE, DELETE};
    
    virtual ~OperationProcessor()
    {
    }
    
    virtual void Append(int op, const PMDocumentType& doc)
    {
    }
    
    virtual void ScdEnd()
    {
    }
    
    virtual void Finish()
    {
    }
    
};

}

#endif

