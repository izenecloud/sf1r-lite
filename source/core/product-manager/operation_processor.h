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
    
    virtual bool Finish()
    {
        return false;
    }
    
    const std::string& GetLastError() const
    {
        return error_;
    }
    
protected:
    std::string error_;
    
};

}

#endif

