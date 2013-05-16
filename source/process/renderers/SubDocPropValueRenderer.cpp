#include "SubDocPropValueRenderer.h"
#include <rapidjson/document.h>
#include <glog/logging.h>
#include <util/driver/readers/JsonReader.h>

namespace sf1r
{
void SubDocPropValueRenderer::renderSubDocPropValue(
    const std::string& propName,
    const std::string& origText,
    izenelib::driver::Value& resourceValue)
{
    if (origText.empty())
        return;
    izenelib::driver::JsonReader reader;
    if(!reader.read(origText, resourceValue))
    {
        resourceValue[propName] = origText;
    }
    /*
    rapidjson::Document doc;
    if (doc.Parse<0>(origText.c_str()).HasParseError())
    {
        //return;
    }
    const rapidjson::Value& subDocs = doc;
    assert(subDocs.IsArray());
    for (rapidjson::Value::ConstValueIterator vit = subDocs.Begin();
            vit != subDocs.End(); vit++)
    {
        izenelib::driver::Value& subDoc = resourceValue();
        for (rapidjson::Value::ConstMemberIterator mit = vit->MemberBegin();
             mit != vit->MemberEnd(); mit++)
        {
            subDoc[mit->name.GetString()]=mit->value.GetString();
        }
    }*/
}
}
