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
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
    }

    bool dd(const PsmAttach& other) const
    {
        return true;
    }

    friend std::ostream& operator<<(std::ostream& out, const PsmAttach& attach)
    {
        return out;
    }
};

class PsmHelper
{
public:
    static bool GetPsmItem(
            ProductTermAnalyzer& analyzer,
            std::map<std::string, std::string>& doc,
            std::string& key,
            std::vector<std::pair<std::string, double> >& doc_vector,
            PsmAttach& attach)
    {
        const std::string& img_url = doc["Img"];
        if (img_url.empty())
        {
            LOG(INFO) << "There is no Img ... " << std::endl;
            return false;
        }
        key = doc["DOCID"];
        analyzer.Analyze(izenelib::util::UString(img_url, izenelib::util::UString::UTF_8).substr(7), doc_vector);
        uint32_t i;
        uint32_t size = img_url.size();
        if (size < 60)
        {
            for (i = 20; i < size - 6; ++i)
            {
                doc_vector.push_back(std::make_pair(img_url.substr(i, 7), 1.0));
            }
        }
        return true;
    }

    static bool GetPsmItemCon(
            ProductTermAnalyzer& analyzer,
            std::map<std::string, std::string>& doc,
            std::string& key,
            std::vector<std::pair<std::string, double> >& doc_vector,
            PsmAttach& attach,
            uint32_t length,
            std::string& img_con_name)
    {
        if (doc[img_con_name].length() <= length)
        {
//          LOG(INFO) << "Content Empty...";
            return false;
        }
        key = doc["DOCID"];
        analyzer.Analyze(izenelib::util::UString(doc[img_con_name], izenelib::util::UString::UTF_8), doc_vector);
        return true;
    }
};

}

#endif
