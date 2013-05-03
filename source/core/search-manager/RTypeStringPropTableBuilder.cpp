#include <common/RTypeStringPropTable.h>
#include <document-manager/DocumentManager.h>
#include "RTypeStringPropTableBuilder.h"
namespace sf1r
{
RTypeStringPropTableBuilder::RTypeStringPropTableBuilder(DocumentManager& documentManagerPtr)
    :documentManagerPtr_(documentManagerPtr)
{
}

boost::shared_ptr<RTypeStringPropTable>& RTypeStringPropTableBuilder::createPropertyTable(
        const std::string& propertyName)
{
    return documentManagerPtr_.getRTypeStringPropTable(propertyName);
}
}
