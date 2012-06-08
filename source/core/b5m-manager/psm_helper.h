#ifndef SF1R_B5MMANAGER_PSMHELPER_H_
#define SF1R_B5MMANAGER_PSMHELPER_H_

#include <string>
#include <vector>
#include <product-manager/product_price.h>
#include <product-manager/product_term_analyzer.h>
#include <boost/serialization/access.hpp>
#include <boost/serialization/serialization.hpp>
#include "b5m_helper.h"

namespace sf1r {

    struct PsmAttach
    {
        ProductPrice price;
        uint32_t id; //id should be different
        uint32_t cid;
        friend class boost::serialization::access;
        template<class Archive>
        void serialize(Archive & ar, const unsigned int version)
        {
            ar & price & id & cid;
        }

        bool dd(const PsmAttach& other) const
        {
            if(cid!=other.cid) return false;
            if(id==other.id) return false;
            double mid1;
            double mid2;
            if(!price.GetMid(mid1)) return false;
            if(!other.price.GetMid(mid2)) return false;
            double max = std::max(mid1, mid2);
            double min = std::min(mid1, mid2);
            if(min<=0.0) return false;
            double ratio = max/min;
            if(ratio>2.2) return false;
            return true;
        }
    };

    class PsmHelper
    {
    public:
        static bool GetPsmItem(const CategoryStringMatcher& cs_matcher, ProductTermAnalyzer& analyzer
          , std::map<std::string, izenelib::util::UString>& doc
          , std::string& key, std::vector<std::pair<std::string, double> >& doc_vector, PsmAttach& attach)
        {
            if(doc["Category"].length()==0 || doc["Title"].length()==0 || doc["Source"].length()==0)
            {
                return false;
            }
            std::string scategory;
            doc["Category"].convertString(scategory, izenelib::util::UString::UTF_8);
            if( !cs_matcher.Match(scategory) )
            {
                return false;
            }
            doc["DOCID"].convertString(key, izenelib::util::UString::UTF_8);
            ProductPrice price;
            price.Parse(doc["Price"]);
            if(!price.Valid() || price.value.first<=0.0 )
            {
                return false;
            }
            analyzer.Analyze(doc["Title"], doc_vector);

            if( doc_vector.empty() )
            {
                return false;
            }

            attach.price = price;
            UString id_str = doc["Source"];
            id_str.append(UString("|", UString::UTF_8));
            id_str.append(doc["Title"]);
            id_str.append(UString("|", UString::UTF_8));
            id_str.append(price.ToUString());
            attach.id = izenelib::util::HashFunction<izenelib::util::UString>::generateHash32(id_str);
            attach.cid = izenelib::util::HashFunction<izenelib::util::UString>::generateHash32(doc["Category"]);
            return true;
        }
    };

}

#endif

