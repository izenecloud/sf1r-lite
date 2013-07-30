#include "SplitPropValueRenderer.h"
#include <common/Keys.h>
#include <configuration-manager/MiningSchema.h>
#include <mining-manager/util/split_ustr.h>

namespace sf1r
{

using namespace izenelib::driver;
using driver::Keys;

void renderUStrValue(
    const izenelib::util::UString& ustr,
    std::string& buffer,
    Value& value
)
{
    ustr.convertString(buffer, izenelib::util::UString::UTF_8);
    value = buffer;
}

SplitPropValueRenderer::SplitPropValueRenderer(const MiningSchema& miningSchema)
{
    const GroupConfigMap& groupConfigMap = miningSchema.group_config_map;

    for (GroupConfigMap::const_iterator it = groupConfigMap.begin();
        it != groupConfigMap.end(); ++it)
    {
        if (it->second.isStringType())
        {
            groupStrProps_.insert(it->first);
        }
    }

    attrProp_ = miningSchema.attr_property.propName;
}

void SplitPropValueRenderer::renderPropValue(
    const std::string& propName,
    const PropertyValue::PropertyValueStrType& origText,
    izenelib::driver::Value& resourceValue
)
{
    if (groupStrProps_.find(propName) != groupStrProps_.end())
    {
        renderGroupValue_(origText, resourceValue);
        return;
    }

    if (propName == attrProp_)
    {
        renderAttrValue_(origText, resourceValue);
        return;
    }

    resourceValue = propstr_to_str(origText);
}

void SplitPropValueRenderer::renderGroupValue_(
    const PropertyValue::PropertyValueStrType& origText,
    izenelib::driver::Value& groupValue
)
{
    typedef std::vector<izenelib::util::UString> GroupNodes;
    typedef std::vector<GroupNodes> GroupPaths;
    GroupPaths groupPaths;
    split_group_path(propstr_to_ustr(origText), groupPaths);

    std::string buffer;
    for (GroupPaths::const_iterator pathIt = groupPaths.begin();
        pathIt != groupPaths.end(); ++pathIt)
    {
        Value& pathValue = groupValue();

        for (GroupNodes::const_iterator nodeIt = pathIt->begin();
            nodeIt != pathIt->end(); ++nodeIt)
        {
            renderUStrValue(*nodeIt, buffer, pathValue());
        }
    }
}

void SplitPropValueRenderer::renderAttrValue_(
    const PropertyValue::PropertyValueStrType& origText,
    izenelib::driver::Value& attrValue
)
{
    typedef std::vector<izenelib::util::UString> AttrValues;
    std::vector<AttrPair> attrPairs;
    split_attr_pair(propstr_to_ustr(origText), attrPairs);

    std::string buffer;
    for (std::vector<AttrPair>::const_iterator pairIt = attrPairs.begin();
        pairIt != attrPairs.end(); ++pairIt)
    {
        Value& pairValue = attrValue();
        renderUStrValue(pairIt->first, buffer, pairValue[Keys::attr_name]);

        Value& renderAttrValues = pairValue[Keys::attr_values];
        const AttrValues& attrValues = pairIt->second;
        for (AttrValues::const_iterator valueIt = attrValues.begin();
            valueIt != attrValues.end(); ++valueIt)
        {
            renderUStrValue(*valueIt, buffer, renderAttrValues());
        }
    }
}

} // namespace sf1r
