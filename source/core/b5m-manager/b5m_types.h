#ifndef SF1R_B5MMANAGER_B5MTYPES_H_
#define SF1R_B5MMANAGER_B5MTYPES_H_

#include <string>
#include <vector>
#include <boost/serialization/access.hpp>
#include <boost/serialization/serialization.hpp>
#include <types.h>
#include <product-manager/product_price.h>
#include <idmlib/similarity/string_similarity.h>


#define NS_SF1R_B5M_BEGIN namespace sf1r{ namespace b5m{
#define NS_SF1R_B5M_END }}

NS_SF1R_B5M_BEGIN
typedef uint32_t term_t;
typedef uint32_t cid_t;
struct Category
{
    Category()
    : cid(0), parent_cid(0), is_parent(true), depth(0), has_spu(false)
    {
    }
    std::string name;
    uint32_t cid;//inner cid starts with 1, not specified by category file.
    uint32_t parent_cid;//also inner cid
    bool is_parent;
    uint32_t depth;
    bool has_spu;
    std::vector<std::string> keywords;
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
        ar & name & cid & parent_cid & is_parent & depth & has_spu;
    }
};
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
struct Product
{
    enum Type {NOTP, PENDING, BOOK, SPU, FASHION, ATTRIB};
    typedef idmlib::sim::StringSimilarity::Object SimObject;
    Product()
    : id(0), cid(0), aweight(0.0), tweight(0.0), score(0.0), type(NOTP)
    {
    }
    uint32_t id;
    std::string spid;
    std::string stitle;
    std::string scategory;
    std::string fcategory; //front-end category
    std::string spic;
    std::string surl;
    std::string smarket_time;
    cid_t cid;
    ProductPrice price;
    std::vector<Attribute> attributes;
    std::vector<Attribute> dattributes; //display attributes
    UString display_attributes;
    UString filter_attributes;
    std::string sbrand;
    double aweight;
    double tweight;
    SimObject title_obj;
    double score;
    //int type;
    Type type;
    std::string why;
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
        ar & spid & stitle & scategory & spic & surl & smarket_time & cid & price & attributes & display_attributes & filter_attributes & sbrand & aweight & tweight & title_obj;
    }

    static bool FCategoryCompare(const Product& p1, const Product& p2)
    {
        return p1.fcategory<p2.fcategory;
    }

    static bool ScoreCompare(const Product& p1, const Product& p2)
    {
        return p1.score>p2.score;
    }

    static bool CidCompare(const Product& p1, const Product& p2)
    {
        return p1.cid<p2.cid;
    }

    std::string GetAttributeValue() const
    {
        std::string result;
        for(uint32_t i=0;i<attributes.size();i++)
        {
            if(!result.empty()) result+=",";
            result+=attributes[i].name;
            result+=":";
            result+=attributes[i].GetValue();
        }
        return result;
    }
};
NS_SF1R_B5M_END

#endif

