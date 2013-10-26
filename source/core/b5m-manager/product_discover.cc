#include "product_discover.h"
#include "scd_doc_processor.h"

using namespace sf1r;
using namespace sf1r::b5m;
namespace bfs = boost::filesystem;

ProductDiscover::ProductDiscover(ProductMatcher* matcher)
: matcher_(matcher), model_regex_("[a-zA-Z\\d\\-]{4,}")
{
    error_model_regex_.push_back(boost::regex("^[a-z]+$"));
    error_model_regex_.push_back(boost::regex("\\d{2}\\-\\d{2}"));
    error_model_regex_.push_back(boost::regex("\\d{1,3}\\-\\d{1,2}0"));
    error_model_regex_.push_back(boost::regex("[a-z]*201\\d"));
    error_model_regex_.push_back(boost::regex("201\\d[a-z]*"));
    cregexps_.push_back(boost::regex("^手机数码>手机$"));
    cregexps_.push_back(boost::regex("^手机数码>摄像摄影.*$"));
    cregexps_.push_back(boost::regex("^家用电器>大家电.*$"));
    error_cregexps_.push_back(boost::regex("^.*配件$"));
    matcher_->SetUsePriceSim(false);
}

ProductDiscover::~ProductDiscover()
{
}

bool ProductDiscover::Output_(const std::string& key, const CValue& value) const
{
    std::vector<std::string> vec;
    boost::algorithm::split(vec, key, boost::is_any_of(","));
    std::string category = vec[0];
    std::string brand = vec[1];
    std::string model = vec[2];
    SMap::const_iterator mit = msmap_.find(model);
    if(mit!=msmap_.end()) model = mit->second;
    std::string url = "http://www.b5m.com/spus/"+category+"/"+brand+"/"+model;
    std::string docid = B5MHelper::GetPidByUrl(url);
    double sum = 0.0;
    uint32_t count=0;
    for(uint32_t i=0;i<value.size();i++)
    {
        const ScdDocument& doc = value[i];
        std::string sprice;
        doc.getString("Price", sprice);
        ProductPrice price;
        if(!price.Parse(sprice)) continue;
        double dprice = 0.0;
        price.GetMid(dprice);
        if(dprice<=0.0) continue;
        sum += dprice;
        count++;
    }
    double price = sum/count;
    std::vector<std::string> cvec;
    boost::algorithm::split(cvec, category, boost::is_any_of(">"));
    const std::string& category_leaf = cvec.back();

    Document doc;
    doc.property("DOCID") = str_to_propstr(docid);
    doc.property("Price") = price;
    doc.property("Url") = str_to_propstr(url);
    std::string title = brand+" "+model+" "+category_leaf;
    doc.property("OTitle") = value.front().property("Title");
    doc.property("Title") = str_to_propstr(title);
    doc.property("Category") = str_to_propstr(category);
    std::string attrib = "品牌:"+brand+",型号:"+model;
    doc.property("Attribute") = str_to_propstr(attrib);
    writer_->Append(doc);
    return true;
}

bool ProductDiscover::CValid_(const std::string& key, const CValue& value) const
{
    uint32_t count=0;
    for(uint32_t i=0;i<value.size();i++)
    {
        const ScdDocument& doc = value[i];
        std::string sprice;
        doc.getString("Price", sprice);
        ProductPrice price;
        if(!price.Parse(sprice)) continue;
        double dprice = 0.0;
        price.GetMid(dprice);
        if(dprice<=0.0) continue;
        count++;
    }
    if(count<5) return false;
    return true;
}

bool ProductDiscover::Process(const std::string& scd_path, const std::string& output_path, int thread_num)
{
    {
        std::string spu_scd = scd_path+"/SPU.SCD";
        ScdDocProcessor processor(boost::bind(&ProductDiscover::ProcessSPU_, this, _1), 1);
        processor.AddInput(spu_scd);
        processor.Process();
        std::cout<<"SPU map size "<<map_.size()<<std::endl;
    }
    {
        std::string offer_scd = scd_path+"/OFFER.SCD";
        ScdDocProcessor processor(boost::bind(&ProductDiscover::Process_, this, _1), thread_num);
        processor.AddInput(offer_scd);
        processor.Process();
        std::cout<<"candidate map size "<<cmap_.size()<<std::endl;
    }
    if(!boost::filesystem::exists(output_path))
    {
        boost::filesystem::create_directories(output_path);
    }
    writer_.reset(new ScdWriter(output_path, UPDATE_SCD));
    for(CMap::const_iterator it = cmap_.begin(); it!=cmap_.end(); it++)
    {
        if(CValid_(it->first, it->second))
        {
            Output_(it->first, it->second);
        }
    }
    writer_->Close();
    return true;
}

void ProductDiscover::ProcessSPU_(ScdDocument& doc)
{
    std::string category;
    doc.getString("Category", category);
    if(!ValidCategory_(category)) return;
    UString attrib;
    doc.getString("Attribute", attrib);
    std::vector<Attribute> attributes;
    ProductMatcher::ParseAttributes(attrib, attributes);
    std::vector<std::string> brands;
    std::vector<std::string> models;
    for(uint32_t i=0;i<attributes.size();i++)
    {
        if(attributes[i].name=="品牌") 
        {
            std::string avalue = attributes[i].GetValue();
            brands = attributes[i].values;
            for(uint32_t a=0;a<attributes[i].values.size();a++)
            {
                boost::algorithm::to_lower(attributes[i].values[a]);
            }
            for(uint32_t a=0;a<attributes[i].values.size();a++)
            {
                smap_[attributes[i].values[a]] = avalue;
            }
            brands.clear();
            brands.push_back(avalue);
        }
        else if(attributes[i].name=="型号") models = attributes[i].values;
    }
    if(brands.empty()||models.empty()) return;
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
    Product product;
    matcher_->Process(doc, product);
    if(!product.spid.empty()&&!product.stitle.empty()) return;
    std::vector<std::string> brands;
    std::vector<std::string> models;
    GetBrandAndModel_(doc, brands, models);
    if(brands.empty()||models.empty()) return;
    Insert_(doc, brands, models);
}

bool ProductDiscover::ValidCategory_(const std::string& category) const
{
    for(uint32_t i=0;i<error_cregexps_.size();i++)
    {
        if(boost::regex_match(category, error_cregexps_[i])) return false;
    }
    for(uint32_t i=0;i<cregexps_.size();i++)
    {
        if(boost::regex_match(category, cregexps_[i])) return true;
    }
    return false;
}

void ProductDiscover::GetBrandAndModel_(const ScdDocument& doc, std::vector<std::string>& brands, std::vector<std::string>& models)
{
    std::string otitle;
    doc.getString("Title", otitle);
    std::string title = boost::algorithm::to_lower_copy(otitle);
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
                SMap::const_iterator it = smap_.find(brand);
                if(it!=smap_.end())
                {
                    brand = it->second;
                }
                Key key(category, brand);
                if(map_.find(key)!=map_.end())
                {
                    bool exists = false;
                    for(uint32_t b=0;b<brands.size();b++)
                    {
                        if(brands[b]==brand)
                        {
                            exists = true;
                            break;
                        }
                    }
                    if(!exists) brands.push_back(brand);
                }
            }
        }
    }
    boost::sregex_token_iterator iter(otitle.begin(), otitle.end(), model_regex_, 0);
    boost::sregex_token_iterator end;
    for( ; iter!=end; ++iter)
    {
        std::string ocandidate = *iter;
        std::string candidate = boost::algorithm::to_lower_copy(ocandidate);
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
        boost::unique_lock<boost::mutex> lock(mutex_);
        msmap_[candidate] = ocandidate;
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
            cmap_[key].push_back(doc);
            //CMap::iterator it = cmap_.find(key);
            //if(it==cmap_.end()) cmap_.insert(std::make_pair(key, 1));
            //else
            //{
            //    it->second++;
            //}
        }
    }
}


