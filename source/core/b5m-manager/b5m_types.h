#ifndef SF1R_B5MMANAGER_B5MTYPES_H_
#define SF1R_B5MMANAGER_B5MTYPES_H_

#include <string>
#include <vector>
#include <util/ustring/UString.h>
#include <boost/regex.hpp>


#define NS_SF1R_B5M_BEGIN namespace sf1r{ namespace b5m{
#define NS_SF1R_B5M_END }}

NS_SF1R_B5M_BEGIN
struct Attribute
{
    std::string name;
    std::vector<std::string> values;
    bool is_optional;
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
        ar & name & values & is_optional;
    }
    std::string GetValue() const
    {
        std::string result;
        for(uint32_t i=0;i<values.size();i++)
        {
            if(!result.empty()) result+="/";
            result+=values[i];
        }
        return result;
    }
    std::string GetText() const
    {
        return name+":"+GetValue();
    }
};
struct MatchParameter
{
    MatchParameter()
    {
    }

    MatchParameter(const std::string& category_regex_str)
    :category_regex(category_regex_str)
    {
    }

    void SetCategoryRegex(const std::string& str)
    {
        category_regex = boost::regex(str);
    }

    bool MatchCategory(const std::string& scategory) const
    {
        if(category_regex.empty()) return true;
        return boost::regex_match(scategory, category_regex);
    }

    boost::regex category_regex;
};
NS_SF1R_B5M_END

#endif

