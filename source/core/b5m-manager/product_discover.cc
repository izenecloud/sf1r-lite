#include "product_discover.h"
#include "scd_doc_processor.h"

using namespace sf1r;
namespace bfs = boost::filesystem;

ProductDiscover::ProductDiscover(ProductMatcher* matcher)
: matcher_(matcher), model_regex_("[a-zA-Z\\d\\-]{4,}")
{
    error_model_regex_.push_back(boost::regex("^[a-z]+$"));
    error_model_regex_.push_back(boost::regex("\\d{2}\\-\\d{2}"));
    error_model_regex_.push_back(boost::regex("\\d{1,3}\\-\\d{1,2}0"));
    error_model_regex_.push_back(boost::regex("[a-z]*201\\d"));
    error_model_regex_.push_back(boost::regex("201\\d[a-z]*"));
}

ProductDiscover::~ProductDiscover()
{
}

bool ProductDiscover::Process(const std::string& scd_path, const std::string& output_path)
{
    {
        std::string spu_scd = scd_path+"/SPU.SCD";
        ScdDocProcessor processor(boost::bind(&ProductDiscover::ProcessSPU_, this, _1), 1);
        processor.AddInput(spu_scd);
        processor.Process();
    }
    {
        std::string offer_scd = scd_path+"/OFFER.SCD";
        ScdDocProcessor processor(boost::bind(&ProductDiscover::Process_, this, _1), 8);
        processor.AddInput(offer_scd);
        processor.Process();
    }
    for(CMap::const_iterator it = cmap_.begin(); it!=cmap_.end(); it++)
    {
        if(it->second>=5)
        {
            std::cout<<it->first<<" : "<<it->second<<std::endl;
        }
    }
    return true;
}

void ProductDiscover::ProcessSPU_(ScdDocument& doc)
{
    std::string category;
    doc.getString("Category", category);
    if(!ValidCategory_(category)) return;
    UString attrib;
    doc.getString("Attribute", attrib);
    std::vector<ProductMatcher::Attribute> attributes;
    ProductMatcher::ParseAttributes(attrib, attributes);
    std::vector<std::string> brands;
    std::vector<std::string> models;
    for(uint32_t i=0;i<attributes.size();i++)
    {
        if(attributes[i].name=="品牌") brands = attributes[i].values;
        else if(attributes[i].name=="型号") models = attributes[i].values;
    }
    if(brands.empty()||models.empty()) return;
    for(uint32_t i=0;i<brands.size();i++)
    {
        boost::algorithm::to_lower(brands[i]);
    }
    for(uint32_t i=0;i<models.size();i++)
    {
        boost::algorithm::to_lower(models[i]);
    }
    for(uint32_t i=0;i<brands.size();i++)
    {
        Key key(category, brands[i]);
        Map::iterator it = map_.find(key);
        if(it==map_.end())
        {
            map_.insert(std::make_pair(key, models));
        }
        else
        {
            it->second.insert(it->second.end(), models.begin(), models.end());
        }
    }
    
}

void ProductDiscover::Process_(ScdDocument& doc)
{
    std::string category;
    doc.getString("Category", category);
    if(!ValidCategory_(category)) return;
    ProductMatcher::Product product;
    matcher_->Process(doc, product);
    if(!product.spid.empty()) return;
    std::vector<std::string> brands;
    std::vector<std::string> models;
    GetBrandAndModel_(doc, brands, models);
    if(brands.empty()||models.empty()) return;
    Insert_(doc, brands, models);
}

bool ProductDiscover::ValidCategory_(const std::string& category) const
{
    if(category=="手机通讯>手机") return true;
    return false;
}

void ProductDiscover::GetBrandAndModel_(const ScdDocument& doc, std::vector<std::string>& brands, std::vector<std::string>& models)
{
    std::string title;
    doc.getString("Title", title);
    boost::algorithm::to_lower(title);
    boost::algorithm::trim(title);
    UString text(title, UString::UTF_8);
    
    std::string category;
    doc.getString("Category", category);
    std::vector<ProductMatcher::KeywordTag> keywords;
    matcher_->GetKeywords(text, keywords);
    boost::unordered_set<std::string> kset;
    for(uint32_t i=0;i<keywords.size();i++)
    {
        const ProductMatcher::KeywordTag& tag = keywords[i];
        std::string str;
        tag.text.convertString(str, UString::UTF_8);
        kset.insert(str);
        for(uint32_t j=0;j<tag.attribute_apps.size();j++)
        {
            if(tag.attribute_apps[j].attribute_name=="品牌")
            {
                std::string brand = str;
                Key key(category, brand);
                if(map_.find(key)!=map_.end())
                {
                    brands.push_back(brand);
                }
            }
        }
    }
    boost::sregex_token_iterator iter(title.begin(), title.end(), model_regex_, 0);
    boost::sregex_token_iterator end;
    for( ; iter!=end; ++iter)
    {
        std::string candidate = *iter;
        if(candidate[0]=='-' || candidate[candidate.length()-1]=='-') continue;
        if(kset.find(candidate)!=kset.end()) continue;
        bool error = false;
        for(uint32_t e=0;e<error_model_regex_.size();e++)
        {
            if(boost::regex_match(candidate, error_model_regex_[e]))
            {
                error = true;
                break;
            }
        }
        if(error) continue;
        models.push_back(candidate);
    }
}

void ProductDiscover::Insert_(const ScdDocument& doc, const std::vector<std::string>& brands, const std::vector<std::string>& models)
{
    std::string category;
    doc.getString("Category", category);
    for(uint32_t i=0;i<brands.size();i++)
    {
        for(uint32_t j=0;j<models.size();j++)
        {
            std::string key = category+","+brands[i]+","+models[j];
            boost::unique_lock<boost::mutex> lock(mutex_);
            CMap::iterator it = cmap_.find(key);
            if(it==cmap_.end()) cmap_.insert(std::make_pair(key, 1));
            else
            {
                it->second++;
            }
        }
    }
}


