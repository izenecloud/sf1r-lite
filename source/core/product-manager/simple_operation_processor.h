#ifndef SF1R_PRODUCTMANAGER_SIMPLEOPERATIONPROCESSOR_H
#define SF1R_PRODUCTMANAGER_SIMPLEOPERATIONPROCESSOR_H

#include <common/type_defs.h>

#include <boost/operators.hpp>

#include "pm_def.h"
#include "pm_types.h"
#include "operation_processor.h"


namespace sf1r
{

class SimpleOperationProcessor : public OperationProcessor
{
public:
    SimpleOperationProcessor();

    ~SimpleOperationProcessor();


    void Append(SCD_TYPE op, const PMDocumentType& doc);

    bool Finish();

    void Clear()
    {
        operations_.clear();
    }

    std::vector<std::pair<SCD_TYPE, PMDocumentType> >& Data()
    {
        return operations_;
    }

private:
    std::vector<std::pair<SCD_TYPE, PMDocumentType> > operations_;

};

}

#endif
