/**
 * @file SubDocPropValueRenderer.h
 * @author Kevin Lin <Kevin.Lin@izenesoft.com>
 * @date Created 2013-04-25
 */
#ifndef SF1R_SUB_DOC_PROP_VALUE_RENDERER_H
#define SF1R_SUB_DOC_PROP_VALUE_RENDERER_H

#include <util/driver/Renderer.h>
#include <util/ustring/UString.h>

#include <string>
#include <set>

namespace sf1r
{

class SubDocPropValueRenderer : public izenelib::driver::Renderer
{
public:

    static void renderSubDocPropValue(
        const std::string& propName,
        const std::string& origText,
        izenelib::driver::Value& resourceValue
    );
};

} // namespace sf1r

#endif // SF1R_SPLIT_PROP_VALUE_RENDERER_H
