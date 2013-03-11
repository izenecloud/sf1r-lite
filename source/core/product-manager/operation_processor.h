#ifndef SF1R_PRODUCTMANAGER_OPERATIONPROCESSOR_H
#define SF1R_PRODUCTMANAGER_OPERATIONPROCESSOR_H

#include <common/type_defs.h>
#include <common/ScdParser.h>

#include <boost/operators.hpp>

#include "pm_def.h"
#include "pm_types.h"


namespace sf1r
{

class OperationProcessor
{
public:
    virtual ~OperationProcessor()
    {
    }
    
    virtual void Append(SCD_TYPE op, const PMDocumentType& doc) = 0;
    
    virtual bool Finish() = 0;
    
    virtual void Clear() = 0;
    
    const std::string& GetLastError() const
    {
        return error_;
    }
    
protected:
    std::string error_;
    
};

}

#endif

