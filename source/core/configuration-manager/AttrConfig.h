#ifndef SF1R_ATTR_CONFIG_H_
#define SF1R_ATTR_CONFIG_H_

#include <util/ustring/UString.h>

#include <string>
#include <set>
#include <boost/serialization/access.hpp>
#include <boost/algorithm/string.hpp>

namespace sf1r
{

/**
 * @brief The type of attr property configuration.
 */
class AttrConfig
{
    struct caseInSensitiveLess : std::binary_function<std::string, std::string, bool>
    {
        bool operator() (const std::string & s1, const std::string & s2) const 
        {
            return boost::algorithm::lexicographical_compare(s1, s2, boost::algorithm::is_iless());
        }
    };
public:
    /// property name
    std::string propName;

    /// attribute names to exclude
    std::set<std::string, caseInSensitiveLess > excludeAttrNames;

    bool isExcludeAttrName(const izenelib::util::UString& attrName) const
    {
        std::string str;
        attrName.convertString(str, izenelib::util::UString::UTF_8);

        return excludeAttrNames.find(str) != excludeAttrNames.end();
    }

private:
    friend class boost::serialization::access;

    template <typename Archive>
    void serialize( Archive & ar, const unsigned int version )
    {
        ar & propName;
        ar & excludeAttrNames;
    }
};

} // namespace

#endif //_ATTR_CONFIG_H_
