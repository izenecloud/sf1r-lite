#ifndef SF1V5_COLLECTIONPARAMETERCONFIG_H_
#define SF1V5_COLLECTIONPARAMETERCONFIG_H_

#include <string>
#include <boost/serialization/access.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/split.hpp>
#include <util/ticpp/ticpp.h>
#include <set>
#include <am/3rdparty/rde_hash.h>
#include <boost/lexical_cast.hpp>

namespace sf1r
{
namespace ticpp = izenelib::util::ticpp;
class CollectionParameterConfig
{
public:
    CollectionParameterConfig();
    ~CollectionParameterConfig();

    void LoadXML(const ticpp::Element * element, bool reload = false);

    void Insert(const std::string& key, const std::string& value);

    void Update(const std::string& key, const std::string& value);

    bool GetString(const std::string& key, std::string& value);

    void GetString(const std::string& key, std::string& value, const std::string& default_value);

    template <class T>
    bool Get(const std::string& key, T& value)
    {
        std::string str_value;
        bool b = value_.get(key, str_value);
        if (!b) return false;
        value = boost::lexical_cast<T>(str_value);
        return true;
    }

    bool Get(const std::string& key, bool& value);

    bool Get(const std::string& key, std::vector<std::string>& value);

    bool Get(const std::string& key, std::set<std::string>& value);

private:
    izenelib::am::rde_hash<std::string, std::string> value_;
}; // end - CollectionParameterConfig

} // end - namespace sf1r

#endif

