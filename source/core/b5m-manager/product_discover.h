#ifndef SF1R_B5MMANAGER_PRODUCTDISCOVER_H_
#define SF1R_B5MMANAGER_PRODUCTDISCOVER_H_
#include "b5m_helper.h"
#include "b5m_types.h"
#include "product_matcher.h"

namespace sf1r {

    using izenelib::util::UString;

    class ProductDiscover {
    public:
        typedef std::pair<std::string, std::string> Key;
        typedef std::vector<std::string> Value;
        typedef boost::unordered_map<Key, Value> Map;
        typedef boost::unordered_map<std::string, std::size_t> CMap;
        ProductDiscover(ProductMatcher* matcher=NULL);
        ~ProductDiscover();
        bool Process(const std::string& scd_path, const std::string& output_path);
    private:
        bool ValidCategory_(const std::string& category) const;
        void Process_(ScdDocument& doc);
        void ProcessSPU_(ScdDocument& doc);
        void GetBrandAndModel_(const ScdDocument& doc, std::vector<std::string>& brands, std::vector<std::string>& models);
        void Insert_(const ScdDocument& doc, const std::vector<std::string>& brands, const std::vector<std::string>& models);

    private:
        ProductMatcher* matcher_;
        Map map_;
        CMap cmap_;
        boost::regex model_regex_;
        std::vector<boost::regex> error_model_regex_;
        boost::mutex mutex_;
    };
}

#endif

