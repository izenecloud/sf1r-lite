#ifndef SF1R_B5MMANAGER_PRODUCTDISCOVER_H_
#define SF1R_B5MMANAGER_PRODUCTDISCOVER_H_
#include "b5m_helper.h"
#include "b5m_types.h"
#include "product_matcher.h"
#include <common/ScdWriter.h>
#include <boost/shared_ptr.hpp>

NS_SF1R_B5M_BEGIN

using izenelib::util::UString;

class ProductDiscover {
public:
    typedef std::pair<std::string, std::string> Key;
    typedef std::vector<std::string> Value;
    typedef boost::unordered_map<Key, Value> Map;
    typedef std::vector<ScdDocument> CValue;
    typedef boost::unordered_map<std::string, CValue> CMap;
    typedef boost::unordered_map<std::string, std::string> SMap;
    ProductDiscover(ProductMatcher* matcher=NULL);
    ~ProductDiscover();
    bool Process(const std::string& scd_path, const std::string& output_path, int thread_num=1);
private:
    bool ValidCategory_(const std::string& category) const;
    void Process_(ScdDocument& doc);
    void ProcessSPU_(ScdDocument& doc);
    void GetBrandAndModel_(const ScdDocument& doc, std::vector<std::string>& brands, std::vector<std::string>& models);
    void Insert_(const ScdDocument& doc, const std::vector<std::string>& brands, const std::vector<std::string>& models);
    bool CValid_(const std::string& key, const CValue& value) const;
    bool Output_(const std::string& key, const CValue& value) const;

private:
    ProductMatcher* matcher_;
    Map map_;
    CMap cmap_;
    SMap smap_;
    SMap msmap_;
    boost::shared_ptr<ScdWriter> writer_;
    std::vector<boost::regex> cregexps_;
    std::vector<boost::regex> error_cregexps_;
    boost::regex model_regex_;
    std::vector<boost::regex> error_model_regex_;
    boost::mutex mutex_;
};
NS_SF1R_B5M_END

#endif

