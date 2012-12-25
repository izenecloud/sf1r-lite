#ifndef SF1R_B5MMANAGER_IMG_DUP_HELPER_H_
#define SF1R_B5MMANAGER_IMG_DUP_HELPER_H_

#include <string>
#include <vector>
#include <product-manager/product_price.h>
#include <product-manager/product_term_analyzer.h>
#include <boost/serialization/access.hpp>
#include <boost/serialization/serialization.hpp>
#include "b5m_helper.h"
#include <boost/regex.hpp>


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
            return true;
        }
        friend std::ostream& operator<<(std::ostream& out, const PsmAttach& attach)
        {
            out<<"price "<<attach.price.ToString()<<", id "<<attach.id<<", cid "<<attach.cid;
            return out;
        }
    };

    class PsmHelper
    {
    public:
        static bool GetPsmItem(ProductTermAnalyzer& analyzer
          , std::map<std::string, izenelib::util::UString>& doc
          , std::string& key, std::vector<std::pair<std::string, double> >& doc_vector, PsmAttach& attach)
        {
            if(doc["Img"].length()==0)
            {
                LOG(INFO)<<"There is no Img ... "<<std::endl;
                return false;
            }
            doc["DOCID"].convertString(key, izenelib::util::UString::UTF_8);

            analyzer.Analyze(doc["Img"], doc_vector);

            return true;
        }

    };

}

#endif
