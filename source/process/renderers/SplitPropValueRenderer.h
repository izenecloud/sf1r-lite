/**
 * @file SplitPropValueRenderer.h
 * @author Jun Jiang <jun.jiang@izenesoft.com>
 * @date Created 2012-08-16
 */
#ifndef SF1R_SPLIT_PROP_VALUE_RENDERER_H
#define SF1R_SPLIT_PROP_VALUE_RENDERER_H

#include <util/driver/Renderer.h>
#include <util/ustring/UString.h>
#include <common/PropertyValue.h>

#include <string>
#include <set>

namespace sf1r
{
class MiningSchema;

class SplitPropValueRenderer : public izenelib::driver::Renderer
{
public:
    SplitPropValueRenderer(const MiningSchema& miningSchema);

    void renderPropValue(
        const std::string& propName,
        const PropertyValue::PropertyValueStrType& origText,
        izenelib::driver::Value& resourceValue
    );

private:
    void renderGroupValue_(
        const PropertyValue::PropertyValueStrType& origText,
        izenelib::driver::Value& groupValue
    );

    void renderAttrValue_(
        const PropertyValue::PropertyValueStrType& origText,
        izenelib::driver::Value& attrValue
    );

private:
    std::set<std::string> groupStrProps_;

    std::string attrProp_;
};

} // namespace sf1r

#endif // SF1R_SPLIT_PROP_VALUE_RENDERER_H
