#ifndef SF1R_PRODUCTMANAGER_SCDOPERATIONPROCESSOR_H
#define SF1R_PRODUCTMANAGER_SCDOPERATIONPROCESSOR_H

#include <common/type_defs.h>

#include <boost/operators.hpp>

#include "pm_def.h"
#include "pm_types.h"
#include "operation_processor.h"


namespace sf1r
{
    
class ScdWriter;

class ScdOperationProcessor : public OperationProcessor
{
public:
    
    ScdOperationProcessor(const std::string& dir);
    
    ~ScdOperationProcessor();
    
    
    void Append(int op, const PMDocumentType& doc);
    
    bool Finish();
    
    void Clear();
    
private:
    
    void ClearScds_();
    
    void AfterProcess_(bool is_succ);
    
private:
    std::string dir_;
    ScdWriter* writer_;
    int last_op_;
};

}

#endif

