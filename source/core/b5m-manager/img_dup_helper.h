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
/*
            for(i=6;i<size-2;i+=1)
            {
                doc_vector.push_back(make_pair(img_url.substr(i,3), 0.4));
            }
            for(i=6;i<size-5;i+=4)
            {
                doc_vector.push_back(make_pair(img_url.substr(i,6), 0.7));
            }
            for(i=6;i<size-8;i+=7)
            {
                doc_vector.push_back(make_pair(img_url.substr(i,9), 0.8));
            }
            for(i=6;i<size-11;i+=10)
            {
                doc_vector.push_back(make_pair(img_url.substr(i,12), 0.9));
            }
            for(i=6;i<size-14;i+=13)
            {
                doc_vector.push_back(make_pair(img_url.substr(i,15), 1.0));
            }
            doc_vector.push_back(make_pair(img_url.substr(6, size/3), 1.0));
            doc_vector.push_back(make_pair(img_url.substr(size/3-1, size/3), 1.0));
            doc_vector.push_back(make_pair(img_url.substr(size*2/3-1), 1.0));
            doc_vector.push_back(make_pair(img_url.substr(6, size/2), 1.0));
            doc_vector.push_back(make_pair(img_url.substr(size/2-1), 1.0));
            doc_vector.push_back(make_pair(img_url.substr(6), 1.0));
*/
            return true;
        }

    };

}

#endif
