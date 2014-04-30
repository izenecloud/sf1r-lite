#include "NumericPropertyTableBuilderImpl.h"
#include <document-manager/DocumentManager.h>

using namespace sf1r;

NumericPropertyTableBuilderImpl::NumericPropertyTableBuilderImpl(
    DocumentManager& documentManager)
    : documentManager_(documentManager)
{
}

boost::shared_ptr<NumericPropertyTableBase>& NumericPropertyTableBuilderImpl::createPropertyTable(
    const std::string& propertyName)
{
    return documentManager_.getNumericPropertyTable(propertyName);
}
