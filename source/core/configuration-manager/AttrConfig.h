#ifndef SF1R_ATTR_CONFIG_H_
#define SF1R_ATTR_CONFIG_H_

#include <util/ustring/UString.h>

#include <string>
#include <set>
#include <boost/serialization/access.hpp>

namespace sf1r
{

/**
 * @brief The type of attr property configuration.
 */
class AttrConfig
{
public:
    /// property name
    std::string propName;

    /// attribute names to exclude
    std::set<std::string> excludeAttrNames;

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
