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

        uint8_t id;
        friend class boost::serialization::access;
        template<class Archive>
        void serialize(Archive & ar, const unsigned int version)
        {
            ar & id;
        }

        bool dd(const PsmAttach& other) const
        {
            return true;
        }

        friend std::ostream& operator<<(std::ostream& out, const PsmAttach& attach)
        {
            out<<"id "<<attach.id;
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
                std::string img_url;
                doc["Img"].convertString(img_url, izenelib::util::UString::UTF_8);
                analyzer.Analyze(doc["Img"].substr(7), doc_vector);
                uint32_t i;
                uint32_t size = img_url.size();
                if(size < 60)
                {
                    for(i=20;i<size-6;i++)
                    {
                        doc_vector.push_back(make_pair(img_url.substr(i, 7), 1.0));
                    }
                }
                return true;
            }

            static bool GetPsmItemCon(ProductTermAnalyzer& analyzer
                    , std::map<std::string, izenelib::util::UString>& doc
                    , std::string& key, std::vector<std::pair<std::string, double> >& doc_vector, PsmAttach& attach, uint32_t length)
            {
                if(doc["Content"].length()<=length)
                {
//                    LOG(INFO)<<"Content Empty... "<<std::endl;
                    return false;
                }
                doc["DOCID"].convertString(key, izenelib::util::UString::UTF_8);
                analyzer.Analyze(doc["Content"], doc_vector);
                return true;
            }
    };

}

#endif
