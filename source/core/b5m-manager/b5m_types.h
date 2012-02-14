#ifndef SF1R_B5MMANAGER_B5MTYPES_H_
#define SF1R_B5MMANAGER_B5MTYPES_H_

#include <string>
#include <vector>
#include <util/ustring/UString.h>
#include <boost/regex.hpp>

namespace sf1r {
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
    struct FeatureCondition
    {
        virtual bool operator()(double v) const
        {
            return true;
        }
    };

    struct PositiveFC : public FeatureCondition
    {
        bool operator()(double v) const
        {
            if(v>0.0) return true;
            return false;
        }
    };

    struct NonNegativeFC : public FeatureCondition
    {
        bool operator()(double v) const
        {
            if(v<0.0) return false;
            return true;
        }
    };

    struct NonZeroFC : public FeatureCondition
    {
        bool operator()(double v) const
        {
            if(v==0.0) return false;
            return true;
        }
    };
    typedef uint32_t AttribId;
    typedef uint32_t AttribNameId;
    struct ProductAttrib
    {
        izenelib::util::UString oid;
        std::vector<AttribId> aid_list;
        template<class Archive> void serialize(Archive & ar,
                const unsigned int version) {
            ar & oid & aid_list;
        }
    };

    struct ProductDocument
    {
        std::map<std::string, izenelib::util::UString> property;
        std::vector<AttribId> tag_aid_list;
        std::vector<AttribId> aid_list;
        template<class Archive> void serialize(Archive & ar,
                const unsigned int version) {
            ar & property & tag_aid_list & aid_list;
        }
    };

    typedef izenelib::util::UString AttribRep;
    typedef uint64_t AttribValueId;
}

#endif

