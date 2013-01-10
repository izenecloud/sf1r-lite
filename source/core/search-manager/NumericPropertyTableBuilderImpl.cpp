#include "NumericPropertyTableBuilderImpl.h"
#include <document-manager/DocumentManager.h>
#include <mining-manager/faceted-submanager/ctr_manager.h>

using namespace sf1r;

NumericPropertyTableBuilderImpl::NumericPropertyTableBuilderImpl(
    DocumentManager& documentManager,
    const faceted::CTRManager* ctrManager)
    : documentManager_(documentManager)
{
    if (ctrManager)
    {
        ctrPropTable_ = ctrManager->getPropertyTable();
    }
}

boost::shared_ptr<NumericPropertyTableBase>& NumericPropertyTableBuilderImpl::createPropertyTable(
    const std::string& propertyName)
{
    if (propertyName == faceted::CTRManager::kCtrPropName)
        return ctrPropTable_;

    return documentManager_.getNumericPropertyTable(propertyName);
}
