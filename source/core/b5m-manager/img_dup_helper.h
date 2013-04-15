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
            std::map<std::string, izenelib::util::UString>& doc,
            std::string& key,
            std::vector<std::pair<std::string, double> >& doc_vector,
            PsmAttach& attach)
    {
        const izenelib::util::UString& img_ustr = doc["Img"];
        if (img_ustr.empty())
        {
            LOG(INFO) << "There is no Img ... " << std::endl;
            return false;
        }
        doc["DOCID"].convertString(key, izenelib::util::UString::UTF_8);
        std::string img_url;
        img_ustr.convertString(img_url, izenelib::util::UString::UTF_8);
        analyzer.Analyze(img_ustr.substr(7), doc_vector);
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
            std::map<std::string, izenelib::util::UString>& doc,
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
        doc["DOCID"].convertString(key, izenelib::util::UString::UTF_8);
        analyzer.Analyze(doc[img_con_name], doc_vector);
        return true;
    }
};

}

#endif
