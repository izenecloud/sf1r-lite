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

    struct CategoryStringMatcher
    {
        std::vector<boost::regex> regex_list;
        void AddCategoryRegex(const std::string& r)
        {
            boost::regex reg(r);
            regex_list.push_back(reg);
        }
        bool Match(const std::string& scategory) const
        {
            for(uint32_t i=0;i<regex_list.size();i++)
            {
                if(boost::regex_match(scategory, regex_list[i])) return true;

            }
            return false;
        }
    };

    //struct FeatureCondition
    //{
        //virtual bool operator()(double v) const
        //{
            //return true;
        //}
    //};

    //struct PositiveFC : public FeatureCondition
    //{
        //bool operator()(double v) const
        //{
            //if(v>0.0) return true;
            //return false;
        //}
    //};

    //struct NonNegativeFC : public FeatureCondition
    //{
        //bool operator()(double v) const
        //{
            //if(v<0.0) return false;
            //return true;
        //}
    //};

    //struct NonZeroFC : public FeatureCondition
    //{
        //bool operator()(double v) const
        //{
            //if(v==0.0) return false;
            //return true;
        //}
    //};

    struct FeatureStatus
    {
        FeatureStatus():pnum(0), nnum(0), znum(0)
        {
        }
        uint16_t pnum;//positive number
        uint16_t nnum;//negative number
        uint16_t znum;//zero number
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
    typedef std::vector<std::pair<AttribNameId, double> > FeatureType;

    struct B5MToken
    {
        enum TokenType {TOKEN_A, TOKEN_C, TOKEN_S};
        izenelib::util::UString text;
        TokenType type;

        B5MToken()
        :text(), type(TOKEN_C)
        {
        }

        B5MToken(const izenelib::util::UString& tex, TokenType typ)
        :text(tex), type(typ)
        {
        }
    };

    struct BuueFrom
    {
        BuueFrom(){}

        BuueFrom(const std::string& p1, const std::string& p2)
        :docid(p1), from_uuid(p2)
        {
        }

        std::string docid;
        std::string from_uuid;
    };

    struct UueFromTo
    {
        std::string from;
        std::string to;
    };

    struct UueItem
    {
        std::string docid;
        UueFromTo from_to;
    };

    struct CompareUueFrom
    {
        bool operator()(const UueItem& u1, const UueItem& u2)
        {
            return u1.from_to.from < u2.from_to.from;
        }
    };

    struct CompareUueTo
    {
        bool operator()(const UueItem& u1, const UueItem& u2)
        {
            return u1.from_to.to < u2.from_to.to;
        }
    };

    enum {BUUE_APPEND, BUUE_REMOVE};

    struct BuueItem
    {
        //BuueItem(){}


        //BuueItem(const std::string& p1, const std::string& p2, const std::string& p3)
        //:to_uuid(p1), from(1, BuueFrom(p2, p3))
        //{
        //}

        int type;
        std::string pid;
        std::vector<std::string> docid_list;

        //std::string to_uuid;
        //std::vector<BuueFrom> from;
        //bool operator<(const BuueItem& other) const
        //{
            //return to_uuid<other.to_uuid;
        //}

        //BuueItem& operator+=(const BuueItem& other)
        //{
            //from.insert(from.end(), other.from.begin(), other.from.end());
            //return *this;
        //}
    };



}

#endif

