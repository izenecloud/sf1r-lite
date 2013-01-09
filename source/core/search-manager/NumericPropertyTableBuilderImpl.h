/**
 * @file NumericPropertyTableBuilderImpl.h
 * @brief implementation to create NumericPropertyTableBase instance.
 * @author Jun Jiang
 * @date Created 2013-01-06
 */

#ifndef SF1R_NUMERIC_PROPERTY_TABLE_BUILDER_IMPL_H
#define SF1R_NUMERIC_PROPERTY_TABLE_BUILDER_IMPL_H

#include "NumericPropertyTableBuilder.h"

namespace sf1r
{
class DocumentManager;
namespace faceted { class CTRManager; }

class NumericPropertyTableBuilderImpl : public NumericPropertyTableBuilder
{
public:
    NumericPropertyTableBuilderImpl(
        DocumentManager& documentManager,
        const faceted::CTRManager* ctrManager);

    virtual boost::shared_ptr<NumericPropertyTableBase>& createPropertyTable(
        const std::string& propertyName);

private:
    DocumentManager& documentManager_;

    boost::shared_ptr<NumericPropertyTableBase> ctrPropTable_;
};

} // namespace sf1r

#endif // SF1R_NUMERIC_PROPERTY_TABLE_BUILDER_IMPL_H
