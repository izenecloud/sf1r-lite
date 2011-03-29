#include "CollectionParameterConfig.h"

namespace sf1r
{
namespace ticpp = izenelib::util::ticpp;
CollectionParameterConfig::CollectionParameterConfig()  {}
CollectionParameterConfig::~CollectionParameterConfig() {}

void CollectionParameterConfig::LoadXML(const ticpp::Element * element, bool reload)
{
    ticpp::Iterator<ticpp::Element> it;

    bool firstcdd = true;
    std::string cddkey = "CollectionDataDirectory";
    for ( it = it.begin( element ); it != it.end(); it++ )
    {
        ticpp::Element * param_node = it.Get();
        std::string node_name = param_node->Value();
        if ( node_name == cddkey )
        {
            if ( firstcdd && reload )
            {
                Update(cddkey, "");
                firstcdd = false;
            }
            else
            {
                std::string dir_name = param_node->GetText(false);
                std::string cddvalue = "";
                bool b = GetString(cddkey, cddvalue);
                if ( !b || cddvalue == "")
                {
                    Update(cddkey, dir_name);
                }
                else
                {
                    Update(cddkey, cddvalue+"|"+dir_name);
                }
            }
        }
        else
        {
            ticpp::Attribute* attrib = param_node->FirstAttribute(false);
            while ( attrib!= NULL)
            {
                std::string key = node_name+"/"+attrib->Name();
                std::string value = attrib->Value();
//         std::cout<<"[collection-config]"<<key<<","<<value<<std::endl;
                Update(key, value);
                attrib = attrib->Next(false);
            }

        }
    }
}

void CollectionParameterConfig::Insert(const std::string& key, const std::string& value)
{
    value_.insert(key, value);
}

void CollectionParameterConfig::Update(const std::string& key, const std::string& value)
{
    value_.del(key);
    value_.insert(key, value);
}

bool CollectionParameterConfig::GetString(const std::string& key, std::string& value)
{
    return value_.get(key, value);
}

void CollectionParameterConfig::GetString(const std::string& key, std::string& value, const std::string& default_value)
{
    bool found = value_.get(key, value);
    if (found) return;
    value = default_value;
}

bool CollectionParameterConfig::Get(const std::string& key, bool& value)
{
    std::string str_value;
    bool b = value_.get(key, str_value);
    if (!b) return false;
    boost::to_lower(str_value);
    if ( str_value == "y" || str_value == "yes" )
    {
        value = true;
    }
    else
    {
        value = false;
    }
    return true;
}

bool CollectionParameterConfig::Get(const std::string& key, std::vector<std::string>& value)
{
    std::string str_value;
    bool b = value_.get(key, str_value);
    if (!b) return false;
    boost::algorithm::split( value, str_value, boost::algorithm::is_any_of("|") );
    return true;
}

bool CollectionParameterConfig::Get(const std::string& key, std::set<std::string>& value)
{
    std::string str_value;
    bool b = value_.get(key, str_value);
    if (!b) return false;
    std::vector<std::string> vec_value;
    boost::algorithm::split( vec_value, str_value, boost::algorithm::is_any_of("|") );
    for (uint32_t i=0;i<vec_value.size();i++)
    {
        value.insert(vec_value[i]);
    }
    return true;
}
}
