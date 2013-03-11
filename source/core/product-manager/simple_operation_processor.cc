#include "simple_operation_processor.h"

using namespace sf1r;

SimpleOperationProcessor::SimpleOperationProcessor()
{
}

SimpleOperationProcessor::~SimpleOperationProcessor()
{
}


void SimpleOperationProcessor::Append(SCD_TYPE op, const PMDocumentType& doc)
{
    operations_.push_back(std::make_pair(op, doc));
}

bool SimpleOperationProcessor::Finish()
{
    return true;
}
