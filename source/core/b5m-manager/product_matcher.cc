#include "product_matcher.h"
#include <util/hashFunction.h>
#include <common/ScdParser.h>
#include <common/ScdWriter.h>
#include <mining-manager/util/split_ustr.h>
#include <product-manager/product_price.h>
#include <boost/unordered_set.hpp>
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <algorithm>
#include <cmath>
#include <idmlib/util/svm.h>
#include <util/functional.h>
#include <util/ClockTimer.h>
#include <3rdparty/udt/md5.h>
using namespace sf1r;
using namespace idmlib::sim;
using namespace idmlib::kpe;
using namespace idmlib::util;

#define Malloc(type,n) (type *)malloc((n)*sizeof(type))

//#define B5M_DEBUG

ProductMatcher::KeywordTag::KeywordTag():cweight(0.0), aweight(0.0), ngram(1)
{
}

void ProductMatcher::KeywordTag::Flush()
{
    SortAndUnique(category_name_apps);
    SortAndUnique(attribute_apps);
    SortAndUnique(spu_title_apps);
}
void ProductMatcher::KeywordTag::Append(const KeywordTag& another, bool is_complete)
{
    uint32_t osize = category_name_apps.size();
    for(uint32_t i=0;i<another.category_name_apps.size();i++)
    {
        CategoryNameApp aapp = another.category_name_apps[i];
        aapp.is_complete = is_complete;
        bool same_cid = false;
        for(uint32_t j=0;j<osize;j++)
        {
            CategoryNameApp& app = category_name_apps[j];
            if(app.cid==aapp.cid)
            {
                if(!app.is_complete&&aapp.is_complete)
                {
                    app = aapp;
                }
                else if( app.is_complete==aapp.is_complete && aapp.depth<app.depth )
                {
                    app = aapp;
                }
                same_cid = true;
                break;
            }
        }
        if(!same_cid)
        {
            category_name_apps.push_back(aapp);
        }
        //if(app.find(suffixes[s])==app.end())
        //{
            //trie[suffixes[s]].category_name_apps.push_back(cn_app);
        //}
        //else
        //{
            //std::pair<bool, uint32_t>& last = app[suffixes[s]];
            //if(! (last.first && !cn_app.is_complete))
            //{
                //last.first = cn_app.is_complete;
                //last.second = cn_app.depth;
                //trie[suffixes[s]].category_name_apps.back() = cn_app;
            //}
        //}
    }
    //category_name_apps.insert(category_name_apps.end(), another.category_name_apps.begin(), another.category_name_apps.end());
    //for(uint32_t i=osize;i<category_name_apps.size();i++)
    //{
        //if(!is_complete)
        //{
            //category_name_apps[i].is_complete = is_complete;
        //}
    //}
    if(is_complete)
    {
        attribute_apps.insert(attribute_apps.end(), another.attribute_apps.begin(), another.attribute_apps.end());
    }
    spu_title_apps.insert(spu_title_apps.end(), another.spu_title_apps.begin(), another.spu_title_apps.end());
}
bool ProductMatcher::KeywordTag::Combine(const KeywordTag& another)
{
    //check if positions were overlapped
    for(uint32_t i=0;i<another.positions.size();i++)
    {
        const Position& ap = another.positions[i];
        //std::cerr<<"ap "<<ap.first<<","<<ap.second<<std::endl;
        for(uint32_t j=0;j<positions.size();j++)
        {
            const Position& p = positions[j];
            //std::cerr<<"p "<<p.first<<","<<p.second<<std::endl;
            bool _overlapped = true;
            if( ap.first>=p.second || p.first>=ap.second) _overlapped = false;
            if(_overlapped)
            {
                return false;
            }
        }
    }
    category_name_apps.clear();
    std::vector<AttributeApp> new_attribute_apps;
    new_attribute_apps.reserve(std::max(attribute_apps.size(), another.attribute_apps.size()));

    uint32_t i=0;
    uint32_t j=0;
    while(i<attribute_apps.size()&&j<another.attribute_apps.size())
    {
        AttributeApp& app = attribute_apps[i];
        const AttributeApp& aapp = another.attribute_apps[j];
        if(app.spu_id==aapp.spu_id)
        {
            if(app.attribute_name!=aapp.attribute_name)
            {
                new_attribute_apps.push_back(app);
                new_attribute_apps.push_back(aapp);
                //app.attribute_name += "+"+aapp.attribute_name;
                //app.is_optional = app.is_optional | aapp.is_optional;
            }
            else
            {
                new_attribute_apps.push_back(app);
            }
            //else
            //{
                //app.spu_id = 0;
            //}
            ++i;
            ++j;
        }
        else if(app.spu_id<aapp.spu_id)
        {
            //app.spu_id = 0;
            ++i;
        }
        else
        {
            ++j;
        }
    }
    std::swap(new_attribute_apps, attribute_apps);
    //if(ngram>0) //anytime
    //{
        //while(i<attribute_apps.size()&&j<another.attribute_apps.size())
        //{
            //AttributeApp& app = attribute_apps[i];
            //const AttributeApp& aapp = another.attribute_apps[j];
            //if(app.spu_id==aapp.spu_id)
            //{
                //if(app.attribute_name!=aapp.attribute_name)
                //{
                    //new_attribute_apps.push_back(app);
                    //new_attribute_apps.push_back(aapp);
                    ////app.attribute_name += "+"+aapp.attribute_name;
                    ////app.is_optional = app.is_optional | aapp.is_optional;
                //}
                //else
                //{
                    //new_attribute_apps.push_back(app);
                //}
                ////else
                ////{
                    ////app.spu_id = 0;
                ////}
                //++i;
                //++j;
            //}
            //else if(app.spu_id<aapp.spu_id)
            //{
                ////app.spu_id = 0;
                //++i;
            //}
            //else
            //{
                //++j;
            //}
        //}
        //while(i<attribute_apps.size())
        //{
            //AttributeApp& app = attribute_apps[i];
            //app.spu_id = 0;
            //++i;
        //}
    //}
    i = 0;
    j = 0;
    while(i<spu_title_apps.size()&&j<another.spu_title_apps.size())
    {
        SpuTitleApp& app = spu_title_apps[i];
        const SpuTitleApp& aapp = another.spu_title_apps[j];
        if(app.spu_id==aapp.spu_id)
        {
            app.pstart = std::min(app.pstart, aapp.pstart);
            app.pend = std::max(app.pend, aapp.pend);
            ++i;
            ++j;
        }
        else if(app<aapp)
        {
            app.spu_id = 0;
            ++i;
        }
        else
        {
            ++j;
        }
    }
    while(i<spu_title_apps.size())
    {
        SpuTitleApp& app = spu_title_apps[i];
        app.spu_id = 0;
        ++i;
    }
    //std::vector<AttributeApp> new_attribute_apps;
    //for(uint32_t i=0;i<another.attribute_apps.size();i++)
    //{
        //const AttributeApp& aapp = another.attribute_apps[i];
        //for(uint32_t j=0;j<attribute_apps.size();j++)
        //{
            //const AttributeApp& app = attribute_apps[j];
            //if(aapp.spu_id==app.spu_id && aapp.attribute_name!=app.attribute_name)
            //{
                //AttributeApp new_app;
                //new_app.spu_id = app.spu_id;
                //new_app.attribute_name = app.attribute_name+"+"+aapp.attribute_name;
                //new_app.is_optional = app.is_optional | aapp.is_optional;
                //new_attribute_apps.push_back(new_app);
                //break;
            //}
        //}
    //}
    //std::swap(new_attribute_apps, attribute_apps);

    //std::vector<SpuTitleApp> new_spu_title_apps;
    //for(uint32_t i=0;i<another.spu_title_apps.size();i++)
    //{
        //const SpuTitleApp& aapp = another.spu_title_apps[i];
        //for(uint32_t j=0;j<spu_title_apps.size();j++)
        //{
            //const SpuTitleApp& app = spu_title_apps[j];
            //if(aapp.spu_id==app.spu_id && aapp.pstart!=app.pstart)
            //{
                //SpuTitleApp new_app;
                //new_app.spu_id = app.spu_id;
                //new_app.pstart = std::min(app.pstart, aapp.pstart);
                //new_app.pend = std::max(app.pend, aapp.pend);
                //new_spu_title_apps.push_back(new_app);
                //break;
            //}
        //}
    //}
    //std::swap(new_spu_title_apps, spu_title_apps);
    if(ngram==1)
    {
        cweight = std::min(cweight, another.cweight)*3.0;
    }
    else
    {
        cweight = std::min(cweight, another.cweight)*5.0;
    }
    ++ngram;
    //aweight = std::min(aweight, another.aweight)*ngram;
    positions.insert(positions.end(), another.positions.begin(), another.positions.end());
    return true;
}

ProductMatcher::ProductMatcher(const std::string& path)
:path_(path), is_open_(false), use_price_sim_(true),
 tid_(1), aid_manager_(NULL), analyzer_(NULL), char_analyzer_(NULL), chars_analyzer_(NULL),
 test_docid_("7bc999f5d10830d0c59487bd48a73cae"),
 left_bracket_("("), right_bracket_(")"),
 left_bracket_term_(0), right_bracket_term_(0)
{
}

ProductMatcher::~ProductMatcher()
{
    if(aid_manager_!=NULL)
    {
        aid_manager_->close();
        delete aid_manager_;
    }
    if(analyzer_!=NULL)
    {
        delete analyzer_;
    }
    if(char_analyzer_!=NULL)
    {
        delete char_analyzer_;
    }
    if(chars_analyzer_!=NULL)
    {
        delete chars_analyzer_;
    }
    //if(cr_result_!=NULL)
    //{
        //cr_result_->flush();
        //delete cr_result_;
    //}
}

bool ProductMatcher::IsOpen() const
{
    return is_open_;
}

bool ProductMatcher::Open()
{
    if(!is_open_)
    {
        try
        {
            boost::filesystem::create_directories(path_);
            idmlib::util::IDMAnalyzerConfig aconfig = idmlib::util::IDMAnalyzerConfig::GetCommonConfig("",cma_path_, "");
            aconfig.symbol = true;
            analyzer_ = new idmlib::util::IDMAnalyzer(aconfig);
            idmlib::util::IDMAnalyzerConfig cconfig = idmlib::util::IDMAnalyzerConfig::GetCommonConfig("","", "");
            //cconfig.symbol = true;
            char_analyzer_ = new idmlib::util::IDMAnalyzer(cconfig);
            idmlib::util::IDMAnalyzerConfig csconfig = idmlib::util::IDMAnalyzerConfig::GetCommonConfig("","", "");
            csconfig.symbol = true;
            chars_analyzer_ = new idmlib::util::IDMAnalyzer(csconfig);
            std::string path = path_+"/products";
            izenelib::am::ssf::Util<>::Load(path, products_);
            LOG(INFO)<<"products size "<<products_.size()<<std::endl;
            path = path_+"/category_list";
            izenelib::am::ssf::Util<>::Load(path, category_list_);
            LOG(INFO)<<"category list size "<<category_list_.size()<<std::endl;
            //path = path_+"/keywords";
            //izenelib::am::ssf::Util<>::Load(path, keywords_thirdparty_);
            path = path_+"/category_index";
            izenelib::am::ssf::Util<>::Load(path, category_index_);
            path = path_+"/product_index";
            izenelib::am::ssf::Util<>::Load(path, product_index_);
            path = path_+"/keyword_trie";
            izenelib::am::ssf::Util<>::Load(path, trie_);
            LOG(INFO)<<"trie size "<<trie_.size()<<std::endl;
            //path = path_+"/attrib_id";
            //boost::filesystem::create_directories(path);
            //aid_manager_ = new AttributeIdManager(path+"/id");

            left_bracket_term_ = GetTerm_(left_bracket_);
            right_bracket_term_ = GetTerm_(right_bracket_);
            //std::string logger_file = path_+"/logger";
            //logger_.open(logger_file.c_str(), std::ios::out | std::ios::app );
            //std::string cidset_path = path_+"/cid_set";
            //izenelib::am::ssf::Util<>::Load(cidset_path, cid_set_);
            //std::string a2p_path = path_+"/a2p";
            //izenelib::am::ssf::Util<>::Load(a2p_path, a2p_);
            //std::string category_group_file = path_+"/category_group";
            //if(boost::filesystem::exists(category_group_file))
            //{
                //LoadCategoryGroup(category_group_file);
            //}
            //std::string category_keywords_file = path_+"/category_keywords";
            //if(boost::filesystem::exists(category_keywords_file))
            //{
                //LoadCategoryKeywords_(category_keywords_file);
            //}
            //std::string category_file = path_+"/category.txt";
            //if(boost::filesystem::exists(category_file))
            //{
                //LoadCategories_(category_file);
            //}
        }
        catch(std::exception& ex)
        {
            LOG(ERROR)<<"product matcher open failed"<<std::endl;
            return false;
        }
        is_open_ = true;
    }
    return true;
}

void ProductMatcher::Clear(const std::string& path)
{
    B5MHelper::PrepareEmptyDir(path);
    //if(!boost::filesystem::exists(path)) return;
    //std::vector<std::string> runtime_path;
    //runtime_path.push_back("logger");
    //runtime_path.push_back("attrib_id");
    //runtime_path.push_back("cid_set");
    //runtime_path.push_back("products");
    //runtime_path.push_back("a2p");
    //runtime_path.push_back("category_group");
    //runtime_path.push_back("category_keywords");
    //runtime_path.push_back("category.txt");
    //runtime_path.push_back("cr_result");
    //runtime_path.push_back("category_list");
    //runtime_path.push_back("keywords");
    //runtime_path.push_back("category_index");
    //runtime_path.push_back("keyword_trie");

    //for(uint32_t i=0;i<runtime_path.size();i++)
    //{
        //boost::filesystem::remove_all(path+"/"+runtime_path[i]);
    //}
}
bool ProductMatcher::GetProduct(const std::string& pid, Product& product)
{
    ProductIndex::const_iterator it = product_index_.find(pid);
    if(it==product_index_.end()) return false;
    uint32_t index = it->second;
    if(index>=products_.size()) return false;
    product = products_[index];
    return true;
}

bool ProductMatcher::Index(const std::string& scd_path)
{
    if(!products_.empty())
    {
        std::cout<<"product trained at "<<path_<<std::endl;
        return true;
    }
    //std::string from_file = scd_path+"/category_group";
    //std::string to_file = path_+"/category_group";
    //if(boost::filesystem::exists(from_file))
    //{
        //if(boost::filesystem::exists(to_file))
        //{
            //boost::filesystem::remove_all(to_file);
        //}
        //boost::filesystem::copy_file(from_file, to_file);
        //LoadCategoryGroup(to_file);
    //}
    //from_file = scd_path+"/category_keywords";
    //to_file = path_+"/category_keywords";
    //if(boost::filesystem::exists(from_file))
    //{
        //if(boost::filesystem::exists(to_file))
        //{
            //boost::filesystem::remove_all(to_file);
        //}
        //boost::filesystem::copy_file(from_file, to_file);
        //LoadCategoryKeywords_(to_file);
    //}
    //from_file = scd_path+"/category.txt";
    //to_file = path_+"/category.txt";
    //if(boost::filesystem::exists(from_file))
    //{
        //if(boost::filesystem::exists(to_file))
        //{
            //boost::filesystem::remove_all(to_file);
        //}
        //boost::filesystem::copy_file(from_file, to_file);
        //LoadCategories_(to_file);
    //}
    std::string keywords_file = scd_path+"/keywords.txt";
    if(boost::filesystem::exists(keywords_file))
    {
        std::ifstream ifs(keywords_file.c_str());
        std::string line;
        while( getline(ifs, line))
        {
            boost::algorithm::trim(line);
            keywords_thirdparty_.push_back(line);
        }
        ifs.close();
    }
    std::string not_keywords_file = scd_path+"/not_keywords.txt";
    if(boost::filesystem::exists(not_keywords_file))
    {
        std::ifstream ifs(not_keywords_file.c_str());
        std::string line;
        while( getline(ifs, line))
        {
            boost::algorithm::trim(line);
            TermList tl;
            GetTerms_(line, tl);
            not_keywords_.insert(tl);
        }
        ifs.close();
    }
    std::string scd;
    std::vector<std::string> scd_list;
    ScdParser::getScdList(scd_path, scd_list);
    if(scd_list.size()!=1)
    {
        std::cerr<<"not have 1 training scd";
        return false;
    }
    else
    {
        scd = scd_list[0];
    }
    products_.resize(1);
    category_list_.resize(1);
    ScdParser parser(izenelib::util::UString::UTF_8);
    parser.load(scd);
    uint32_t n=0;
    for( ScdParser::iterator doc_iter = parser.begin();
      doc_iter!= parser.end(); ++doc_iter, ++n)
    {
        //if(n>=15000) break;
        if(n%100000==0)
        {
            LOG(INFO)<<"Find Product Documents "<<n<<std::endl;
        }
        Document doc;
        izenelib::util::UString pid;
        izenelib::util::UString title;
        izenelib::util::UString category;
        izenelib::util::UString attrib_ustr;
        SCDDoc& scddoc = *(*doc_iter);
        SCDDoc::iterator p = scddoc.begin();
        for(; p!=scddoc.end(); ++p)
        {
            const std::string& property_name = p->first;
            doc.property(property_name) = p->second;
            if(property_name=="DOCID")
            {
                pid = p->second;
            }
            else if(property_name=="Title")
            {
                title = p->second;
            }
            else if(property_name=="Category")
            {
                category = p->second;
            }
            else if(property_name=="Attribute")
            {
                attrib_ustr = p->second;
            }
        }
        if(category.length()==0 || attrib_ustr.length()==0 || title.length()==0)
        {
            continue;
        }
        //convert category
        std::string scategory;
        category.convertString(scategory, UString::UTF_8);
        if(category_index_.find(scategory)==category_index_.end())
        {
            uint32_t cid = category_list_.size();
            Category c;
            c.name = scategory;
            c.cid = 0;//not use
            c.parent_cid = 0;//not use
            c.is_parent = false;
            std::vector<std::string> cs_list;
            boost::algorithm::split( cs_list, c.name, boost::algorithm::is_any_of(">") );
            c.depth=cs_list.size();
            category_list_.push_back(c);
            category_index_[scategory] = cid;
        }
        double price = 0.0;
        UString uprice;
        if(doc.getProperty("Price", uprice))
        {
            ProductPrice pp;
            pp.Parse(uprice);
            pp.GetMid(price);
        }
        std::string stitle;
        title.convertString(stitle, izenelib::util::UString::UTF_8);
        std::string spid;
        pid.convertString(spid, UString::UTF_8);
        Product product;
        product.spid = spid;
        product.stitle = stitle;
        product.scategory = scategory;
        product.price = price;
        ParseAttributes_(attrib_ustr, product.attributes);
        uint32_t optional_count = 0;
        uint32_t attrib_count = product.attributes.size();
        //std::cerr<<"[SPU][Title]"<<stitle<<std::endl;
        for(uint32_t i=0;i<product.attributes.size();i++)
        {
            if(product.attributes[i].is_optional)
            {
                optional_count++;
            }
            //std::cerr<<"[SPU][AV]"<<product.attributes[i].values[0]<<","<<product.attributes[i].values[0].length()<<std::endl;
        }
        product.aweight = 1.0*(attrib_count-optional_count)+0.2*optional_count;
        products_.push_back(product);
        product_index_[spid] = products_.size()-1;
    }
    for(uint32_t i=0;i<products_.size();i++)
    {
        Product& p = products_[i];
        string_similarity_.Convert(p.stitle, p.title_obj);
    }
    if(!products_.empty())
    {
        TrieType suffix_trie;
        ConstructSuffixTrie_(suffix_trie);
        ConstructKeywords_();
        LOG(INFO)<<"find "<<keyword_set_.size()<<" keywords"<<std::endl;
        ConstructKeywordTrie_(suffix_trie);
    }
    std::string path = path_+"/products";
    izenelib::am::ssf::Util<>::Save(path, products_);
    path = path_+"/category_list";
    LOG(INFO)<<"category list size "<<category_list_.size()<<std::endl;
    izenelib::am::ssf::Util<>::Save(path, category_list_);
    //path = path_+"/keywords";
    //izenelib::am::ssf::Util<>::Save(path, keywords_thirdparty_);
    path = path_+"/category_index";
    izenelib::am::ssf::Util<>::Save(path, category_index_);
    path = path_+"/product_index";
    izenelib::am::ssf::Util<>::Save(path, product_index_);
    path = path_+"/keyword_trie";
    izenelib::am::ssf::Util<>::Save(path, trie_);
    
    return true;
}


bool ProductMatcher::DoMatch(const std::string& scd_path)
{
    izenelib::util::ClockTimer clocker;
    std::vector<std::string> scd_list;
    B5MHelper::GetIUScdList(scd_path, scd_list);
    if(scd_list.empty()) return false;
    std::string match_file = path_+"/match";
    std::ofstream ofs(match_file.c_str());
    for(uint32_t i=0;i<scd_list.size();i++)
    {
        std::string scd_file = scd_list[i];
        LOG(INFO)<<"Processing "<<scd_file<<std::endl;
        ScdParser parser(izenelib::util::UString::UTF_8);
        parser.load(scd_file);
        uint32_t n=0;
        for( ScdParser::iterator doc_iter = parser.begin();
          doc_iter!= parser.end(); ++doc_iter, ++n)
        {
            if(n%1000==0)
            {
                LOG(INFO)<<"Find Offer Documents "<<n<<std::endl;
            }
            if(n==150000) break;
            SCDDoc& scddoc = *(*doc_iter);
            SCDDoc::iterator p = scddoc.begin();
            Document doc;
            for(; p!=scddoc.end(); ++p)
            {
                const std::string& property_name = p->first;
                doc.property(property_name) = p->second;
            }
            Category result_category;
            Product result_product;
            Process(doc, result_category, result_product);
            std::string spid = result_product.spid;
            std::string sptitle = result_product.stitle;
            std::string soid;
            std::string stitle;
            doc.getString("DOCID", soid);
            doc.getString("Title", stitle);
            if(spid.length()>0)
            {
                ofs<<soid<<","<<spid<<","<<stitle<<","<<sptitle<<std::endl;
            }
            else
            {
                ofs<<soid<<","<<stitle<<std::endl;
            }
            //Product product;
            //if(GetMatched(doc, product))
            //{
                //UString oid;
                //doc.getProperty("DOCID", oid);
                //std::string soid;
                //oid.convertString(soid, izenelib::util::UString::UTF_8);
                //ofs<<soid<<","<<product.spid;
                //boost::posix_time::ptime now = boost::posix_time::second_clock::local_time();
                //ofs<<","<<boost::posix_time::to_iso_string(now);
                //std::string stitle;
                //title.convertString(stitle, UString::UTF_8);
                //ofs<<","<<stitle<<","<<product.stitle<<std::endl;
            //}
            //else
            //{
                ////std::cerr<<"not got pid"<<std::endl;
            //}
        }
    }
    ofs.close();
    LOG(INFO)<<"clocker used "<<clocker.elapsed()<<std::endl;
    return true;
}
bool ProductMatcher::ProcessBook_(const Document& doc, Product& result_product)
{
    std::string scategory;
    doc.getString("Category", scategory);
    if(boost::algorithm::starts_with(scategory, "书籍/杂志/报纸"))
    {
        const static std::string isbn_name = "isbn";
        std::string isbn_value;
        UString attrib_ustr;
        doc.getProperty("Attribute", attrib_ustr);
        std::vector<Attribute> attributes;
        ParseAttributes_(attrib_ustr, attributes);
        for(uint32_t i=0;i<attributes.size();i++)
        {
            const Attribute& a = attributes[i];
            std::string aname = a.name;
            boost::algorithm::trim(aname);
            boost::to_lower(aname);
            if(aname==isbn_name)
            {
                if(!a.values.empty())
                {
                    isbn_value = a.values[0];
                    boost::algorithm::replace_all(isbn_value, "-", "");
                }
                break;
            }
        }
        if(!isbn_value.empty())
        {
            static const int MD5_DIGEST_LENGTH = 32;
            std::string url = "http://www.taobao.com/spuid/isbn-"+isbn_value;

            md5_state_t st;
            md5_init(&st);
            md5_append(&st, (const md5_byte_t*)(url.c_str()), url.size());
            md5_byte_t digest[MD5_DIGEST_LENGTH];
            memset(digest, 0, sizeof(digest));
            md5_finish(&st,digest);
            uint128_t md5_int_value = *((uint128_t*)digest);

            //uint128_t pid = izenelib::util::HashFunction<UString>::generateHash128(UString(pid_str, UString::UTF_8));
            result_product.spid = B5MHelper::Uint128ToString(md5_int_value);
        }
        return true;
    }
    return false;
}

bool ProductMatcher::Process(const Document& doc, Category& result_category, Product& result_product)
{
    if(ProcessBook_(doc, result_product))
    {
        return true;
    }
    izenelib::util::UString title;
    izenelib::util::UString category;
    doc.getProperty("Category", category);
    doc.getProperty("Title", title);
    
    if(title.length()==0)
    {
        return false;
    }
    //keyword_vector_.resize(0);
    //std::cout<<"[TITLE]"<<stitle<<std::endl;
    uint32_t cid = 0;
    uint32_t pid = 0;
    TermList term_list;
    GetCRTerms_(title, term_list);
    KeywordVector keyword_vector;
    GetKeywordVector_(term_list, keyword_vector);
    Compute_(doc, term_list, keyword_vector, cid, pid);
    if(category.empty()&&cid>0)
    {
        result_category = category_list_[cid];
        //std::string scategory = category_list_[cid].name;
        //doc.property("Category") = UString(scategory, UString::UTF_8);
    }
    if(pid>0)
    {
        result_product = products_[pid];
        //std::string spid = products_[pid].spid;
        //doc.property("uuid") = UString(spid, UString::UTF_8);
    }
    //for(KeywordMap::const_iterator it = keyword_map.begin();it!=keyword_map.end();++it)
    //{
        //std::string text = GetText_(it->first);
        //std::cout<<"[KEYWORD]"<<text<<std::endl;
    //}
    return true;
}

void ProductMatcher::GetKeywordVector_(const TermList& term_list, KeywordVector& keyword_vector)
{
    //std::string stitle;
    //text.convertString(stitle, UString::UTF_8);
    uint32_t begin = 0;
    uint8_t bracket_depth = 0;
    keyword_vector.reserve(50);
    typedef boost::unordered_map<TermList, uint32_t> KeywordIndex;
    KeywordIndex keyword_index;
    while(begin<term_list.size())
    {
        term_t begin_term = term_list[begin];
        if(begin_term==left_bracket_term_)
        {
            bracket_depth++;
            begin++;
            continue;
        }
        else if(begin_term==right_bracket_term_)
        {
            if(bracket_depth>0)
            {
                bracket_depth--;
            }
            begin++;
            continue;
        }
        //if(bracket_depth>0)
        //{
            //begin++;
            //continue;
        //}
        bool found_keyword = false;
        std::pair<TrieType::key_type, TrieType::mapped_type> value;
        std::vector<term_t> candidate_keyword(1, begin_term);
        candidate_keyword.reserve(10);
        uint32_t next_pos = begin+1;
        while(true)
        {
            TrieType::const_iterator it = trie_.lower_bound(candidate_keyword);
            if(it!=trie_.end())
            {
                if(candidate_keyword==it->first)
                {
                    value.first = it->first;
                    value.second = it->second;
                    found_keyword = true;
                    KeywordTag& tag = value.second;
                    tag.cweight = 1.0;
                    tag.aweight = 1.0;
                    tag.positions.push_back(std::make_pair(begin, next_pos));
                    if(bracket_depth>0) tag.cweight*=0.01;
                    //if(value.first.size()/term_list.size()>=0.8)
                    //{
                        ////tag.cweight *= 3;
                        //tag.aweight *= 3;
                    //}
                    KeywordIndex::iterator it = keyword_index.find(value.first);
                    if(it==keyword_index.end())
                    {
                        keyword_vector.push_back(value);
                        keyword_index[value.first] = keyword_vector.size()-1;
                    }
                    else
                    {
                        std::pair<TermList, KeywordTag>& evalue = keyword_vector[it->second];
                        if(evalue.second.cweight<tag.cweight)
                        {
                            evalue.second = tag;
                        }
                    }
                }
                else if(StartsWith_(it->first, candidate_keyword))
                {
                    //continue trying
                }
                else
                {
                    break;
                }
            }
            else
            {
                break;
            }
            if(next_pos>=term_list.size()) break;
            candidate_keyword.push_back(term_list[next_pos]);
            ++next_pos;
        }
        

        //if(found_keyword)
        //{
            //value.second.weight = 1.0;
            //if(bracket_depth>0) value.second.weight*=0.2;
            //if(value.first.size()/term_list.size()>=0.8)
            //{
                //value.second.weight *= 3;
            //}
            //KeywordMap::iterator it = keyword_map.find(value.first);
            //if(it==keyword_map.end())
            //{
                //keyword_map.insert(value);
            //}
            //else if(value.second.weight>it->second.weight)
            //{
                //it->second = value.second;
            //}
        //}
        ++begin;
    }
    uint32_t keyword_count = keyword_vector.size();
    for(uint32_t i=0;i<keyword_count;i++)
    {
        for(uint32_t j=i+1;j<keyword_count;j++)
        {
            KeywordTag tag = keyword_vector[i].second;
            if(tag.Combine(keyword_vector[j].second))
            {
                TermList tl(keyword_vector[i].first.begin(), keyword_vector[i].first.end());
                tl.reserve(tl.size()+1+keyword_vector[j].first.size());
                tl.push_back(0);
                tl.insert(tl.end(), keyword_vector[j].first.begin(), keyword_vector[j].first.end());
                //std::string text = GetText_(tl);
                //std::cerr<<"combined "<<text<<std::endl;
                keyword_vector.push_back(std::make_pair(tl, tag));
                for(uint32_t k=j+1;k<keyword_count;k++)
                {
                    KeywordTag ttag(tag);
                    if(ttag.Combine(keyword_vector[k].second))
                    {
                        TermList ttl(tl);
                        ttl.reserve(ttl.size()+1+keyword_vector[k].first.size());
                        ttl.push_back(0);
                        ttl.insert(ttl.end(), keyword_vector[k].first.begin(), keyword_vector[k].first.end());
                        //std::string text = GetText_(ttl);
                        //std::cerr<<"combined "<<text<<std::endl;
                        keyword_vector.push_back(std::make_pair(ttl, ttag));
                    }
                }
            }
        }
    }
}

uint32_t ProductMatcher::GetCidBySpuId_(uint32_t spu_id)
{
    //LOG(INFO)<<"spu_id:"<<spu_id<<std::endl;
    const Product& product = products_[spu_id];
    const std::string& scategory = product.scategory;
    return category_index_[scategory];
}


void ProductMatcher::Compute_(const Document& doc, const TermList& term_list, KeywordVector& keyword_vector, uint32_t& cid, uint32_t& pid)
{
    UString title;
    doc.getProperty("Title", title);
#ifdef B5M_DEBUG
    std::string stitle;
    doc.getString("Title", stitle);
    std::cout<<"[TITLE]"<<stitle<<std::endl;
#endif
    double price = 0.0;
    UString uprice;
    if(doc.getProperty("Price", uprice))
    {
        ProductPrice pp;
        pp.Parse(uprice);
        pp.GetMid(price);
    }
    SimObject title_obj;
    string_similarity_.Convert(title, title_obj);
    //firstly do cid;
    typedef dmap<uint32_t, double> IdToScore;
    typedef std::vector<double> ScoreList;
    typedef dmap<uint32_t, ScoreList> IdToScoreList;
    typedef std::map<uint32_t, ProductCandidate> PidCandidates;
    ScoreList max_share_point(3,0.0);
    max_share_point[0] = 2.0;
    max_share_point[1] = 1.0;
    max_share_point[2] = 1.0;
    IdToScore cid_candidates(0.0);
    //IdToScore pid_candidates(0.0);
    PidCandidates pid_candidates;
    ScoreList default_score_list(3, 0.0);
    IdToScoreList cid_share_list(default_score_list);
    for(KeywordVector::const_iterator it = keyword_vector.begin();it!=keyword_vector.end();++it)
    {
        cid_share_list.clear();
        const TermList& tl = it->first;
        const KeywordTag& tag = it->second;
        double weight = tag.cweight;
        double aweight = tag.aweight;
        double terms_length = tl.size()-(tag.ngram-1);
        if(terms_length/term_list.size()>=0.8)
        {
            for(uint32_t i=0;i<tag.attribute_apps.size();i++)
            {
                if(tag.attribute_apps[i].attribute_name=="型号")
                {
                    aweight*=3.0;
                    break;
                }
            }
        }
#ifdef B5M_DEBUG
        std::string text = GetText_(tl);
        std::cout<<"[KEYWORD]"<<text<<","<<weight<<","<<aweight<<std::endl;
#endif
        //for(uint32_t i=0;i<tag.attribute_apps.size();i++)
        //{
            //const AttributeApp& app = tag.attribute_apps[i];
            //cout<<"[AA]"<<app.spu_id<<","<<app.attribute_name<<std::endl;
        //}
        
        for(uint32_t i=0;i<tag.category_name_apps.size();i++)
        {
            const CategoryNameApp& app = tag.category_name_apps[i];
            double depth_ratio = 1.0-0.2*(app.depth-1);
            double share_point = 2.0*depth_ratio;
            if(!app.is_complete) share_point*=0.5;
#ifdef B5M_DEBUG
            //std::cout<<"[CS]"<<category_list_[app.cid].name<<","<<share_point<<std::endl;
#endif
            cid_share_list[app.cid][0] += share_point;
        }
        for(uint32_t i=0;i<tag.attribute_apps.size();i++)
        {
            const AttributeApp& app = tag.attribute_apps[i];
            if(app.spu_id==0) continue;
            //if(app.is_optional) continue;
            const Product& p = products_[app.spu_id];
#ifdef B5M_DEBUG
            //std::cout<<"[PID]"<<p.spid<<","<<p.stitle<<","<<text<<std::endl;
#endif
            double psim = PriceSim_(price, p.price);
            if(psim<0.25) continue;
            uint32_t _cid = GetCidBySpuId_(app.spu_id);
            double share_point = 0.5;
            share_point*=psim;
            double _aweight = aweight;
            if(app.is_optional) _aweight*=0.2;
            ProductCandidate& pc = pid_candidates[app.spu_id];
            if(pc.attribute_score[app.attribute_name]<_aweight)
            {
                pc.attribute_score[app.attribute_name] = _aweight;
            }
            //double score = 1.0*aweight;
            //if(app.is_optional) score*=0.2;
            //if(score>=p.aweight*0.8)
            //{
                ////double pscore = score*psim;
                //double pscore = score;
//#ifdef B5M_DEBUG
                //std::cout<<"find pid  "<<p.spid<<","<<p.stitle<<std::endl;
//#endif
                //pid_candidates[app.spu_id]+=pscore;
                ////share_point*=2;
            //}
            cid_share_list[_cid][1] += share_point;
        }
        for(uint32_t i=0;i<tag.spu_title_apps.size();i++)
        {
            const SpuTitleApp& app = tag.spu_title_apps[i];
            if(app.spu_id==0) continue;
#ifdef B5M_DEBUG
            //std::cout<<"[TS]"<<products_[app.spu_id].stitle<<std::endl;
#endif
            const Product& p = products_[app.spu_id];
            double psim = PriceSim_(price, p.price);
            uint32_t _cid = GetCidBySpuId_(app.spu_id);
            double share_point = 0.05;
            cid_share_list[_cid][2] += share_point*psim;
        }
        double all_share_point = 0.0;
        for(IdToScoreList::iterator sit = cid_share_list.begin();sit!=cid_share_list.end();sit++)
        {
            //uint32_t cid = sit->first;
            ScoreList& slist = sit->second;
            for(uint32_t i=0;i<slist.size();i++)
            {
                if(slist[i]>max_share_point[i])
                {
                    slist[i] = max_share_point[i];
                }
                all_share_point+=slist[i];
            }
        }
        for(IdToScoreList::iterator sit = cid_share_list.begin();sit!=cid_share_list.end();sit++)
        {
            uint32_t _cid = sit->first;
            ScoreList& slist = sit->second;
            double share_point = 0.0;
#ifdef B5M_DEBUG
            //std::cout<<"[DD]"<<category_list_[_cid].name;
            //for(uint32_t i=0;i<slist.size();i++)
            //{
                //std::cout<<","<<slist[i];
            //}
            //std::cout<<std::endl;
#endif
            for(uint32_t i=0;i<slist.size();i++)
            {
                share_point+=slist[i];
            }
            cid_candidates[_cid] += share_point/all_share_point*weight;
        }
    }
    std::vector<std::pair<double, uint32_t> > cid_vec_candidates;
    GetSortedVector_(cid_candidates, cid_vec_candidates);
    if(cid_vec_candidates.empty()) return;
    cid = cid_vec_candidates[0].second;
    IdToScore possible_cids;
    double max_score = 0.0;
    uint32_t show_count = std::min(5u, (uint32_t)cid_vec_candidates.size());
    for(uint32_t i=0;i<show_count;i++)
    {
        uint32_t _cid = cid_vec_candidates[i].second;
        double score = cid_vec_candidates[i].first;
        if(possible_cids.empty())
        {
            possible_cids.insert(std::make_pair(_cid, score));
            max_score = score;
        }
        else
        {
            double ratio = score/max_score;
            if(ratio>=0.2)
            {
                possible_cids.insert(std::make_pair(_cid, score));
            }
            else
            {
                break;
            }
        }
#ifdef B5M_DEBUG
        std::cout<<"[CC]"<<score<<","<<category_list_[_cid].name<<std::endl;
#endif
    }
    if(max_score==0.0) max_score = 1.0;
    std::string doc_scategory;
    doc.getString("Category", doc_scategory);
    if(!doc_scategory.empty())
    {
        uint32_t doc_cid = category_index_[doc_scategory];
        possible_cids.insert(std::make_pair(doc_cid, max_score));
    }

    std::vector<std::pair<std::pair<double, double>, uint32_t> > pid_vec_candidates; //aweight, common_score(title_sim*category_sim)
    //std::vector<uint32_t> pid_to_erase;
    for(PidCandidates::iterator it = pid_candidates.begin();it!=pid_candidates.end();it++)
    {
        uint32_t spu_id = it->first;
        uint32_t _cid = GetCidBySpuId_(spu_id);
        IdToScore::const_iterator cit = possible_cids.find(_cid);
        if(cit==possible_cids.end())
        {
            //pid_to_erase.push_back(spu_id);
        }
        else
        {
            ProductCandidate& pc = it->second;
            const Product& product = products_[spu_id];
            double aweight = pc.GetAWeight();
            if(aweight>=product.aweight*0.8 && aweight>=1.2)
            {
                double str_sim = string_similarity_.Sim(title_obj, products_[spu_id].title_obj);
                double price_sim = PriceSim_(price, product.price);
                double category_sim = std::sqrt(cit->second);
                double common_sim = str_sim*price_sim*category_sim;
                //double common_sim = str_sim*price_sim;
                pid_vec_candidates.push_back(std::make_pair(std::make_pair(aweight, common_sim), spu_id));
            }
            //it->second *= cit->second*str_sim;
        }
    }
    SortVectorDesc_(pid_vec_candidates);
    //for(uint32_t i=0;i<pid_to_erase.size();i++)
    //{
        //pid_candidates.erase(pid_to_erase[i]);
    //}

    //std::vector<std::pair<double, uint32_t> > pid_vec_candidates;
    //GetSortedVector_(pid_candidates, pid_vec_candidates);
    if(pid_vec_candidates.empty()) return;
    pid = pid_vec_candidates[0].second;
    cid = GetCidBySpuId_(pid);
#ifdef B5M_DEBUG
    show_count = std::min(5u, (uint32_t)pid_vec_candidates.size());
    for(uint32_t i=0;i<show_count;i++)
    {
        std::cout<<"[PP]"<<pid_vec_candidates[i].first.first<<","<<pid_vec_candidates[i].first.second<<","<<products_[pid_vec_candidates[i].second].stitle<<std::endl;
    }
    std::cout<<"[CATEGORY]"<<category_list_[cid].name<<std::endl;
#endif
}

double ProductMatcher::PriceSim_(double offerp, double spup)
{
    if(!use_price_sim_) return 0.25;
    if(spup==0.0) return 0.25;
    if(offerp==0.0) return 0.0;
    if(offerp>spup) return spup/offerp;
    else return offerp/spup;
}

bool ProductMatcher::PriceMatch_(double p1, double p2)
{
    static double min_ratio = 0.25;
    static double max_ratio = 3;
    double ratio = p1/p2;
    return (ratio>=min_ratio)&&(ratio<=max_ratio);
}


void ProductMatcher::Analyze_(const izenelib::util::UString& btext, std::vector<izenelib::util::UString>& result)
{
    AnalyzeImpl_(analyzer_, btext, result);
}

void ProductMatcher::AnalyzeChar_(const izenelib::util::UString& btext, std::vector<izenelib::util::UString>& result)
{
    AnalyzeImpl_(char_analyzer_, btext, result);
}

void ProductMatcher::AnalyzeCR_(const izenelib::util::UString& btext, std::vector<izenelib::util::UString>& result)
{
    izenelib::util::UString text(btext);
    text.toLowerString();
    std::vector<idmlib::util::IDMTerm> term_list;
    chars_analyzer_->GetTermList(text, term_list);
    result.reserve(term_list.size());
    for(std::size_t i=0;i<term_list.size();i++)
    {
        std::string str;
        term_list[i].text.convertString(str, izenelib::util::UString::UTF_8);
        //logger_<<"[A]"<<str<<","<<term_list[i].tag<<std::endl;
        char tag = term_list[i].tag;
        if(tag == idmlib::util::IDMTermTag::SYMBOL)
        {
            if(str=="("||str=="（")
            {
                str = left_bracket_;
            }
            else if(str==")"||str=="）")
            {
                str = right_bracket_;
            }
            else 
            {
                continue;
            }
        }
        result.push_back( izenelib::util::UString(str, izenelib::util::UString::UTF_8) );

    }
}

void ProductMatcher::AnalyzeImpl_(idmlib::util::IDMAnalyzer* analyzer, const izenelib::util::UString& btext, std::vector<izenelib::util::UString>& result)
{
    izenelib::util::UString text(btext);
    text.toLowerString();
    std::vector<idmlib::util::IDMTerm> term_list;
    analyzer->GetTermList(text, term_list);
    result.reserve(term_list.size());
    for(std::size_t i=0;i<term_list.size();i++)
    {
        std::string str;
        term_list[i].text.convertString(str, izenelib::util::UString::UTF_8);
        //logger_<<"[A]"<<str<<","<<term_list[i].tag<<std::endl;
        char tag = term_list[i].tag;
        if(tag == idmlib::util::IDMTermTag::SYMBOL)
        {
            if(str=="-")
            {
                //bool valid = false;
                //if(i>0 && i<term_list.size()-1)
                //{
                    //if(term_list[i-1].tag==idmlib::util::IDMTermTag::NUM
                      //|| term_list[i+1].tag==idmlib::util::IDMTermTag::NUM)
                    //{
                        //valid = true;
                    //}
                //}
                //if(!valid) continue;
            }
            else if(str!="+" && str!=".") continue;
        }
        //if(str=="-") continue;
        boost::to_lower(str);
        //boost::unordered_map<std::string, std::string>::iterator it = synonym_map_.find(str);
        //if(it!=synonym_map_.end())
        //{
            //str = it->second;
        //}
        result.push_back( izenelib::util::UString(str, izenelib::util::UString::UTF_8) );

    }
    //{
        //std::string sav;
        //text.convertString(sav, izenelib::util::UString::UTF_8);
        //logger_<<"[O]"<<sav<<std::endl;
        //logger_<<"[T]";
        //for(std::size_t i=0;i<term_list.size();i++)
        //{
            //std::string s;
            //term_list[i].text.convertString(s, izenelib::util::UString::UTF_8);
            //logger_<<s<<":"<<term_list[i].tag<<",";
        //}
        //logger_<<std::endl;
    //}
}


void ProductMatcher::ParseAttributes_(const UString& ustr, std::vector<Attribute>& attributes)
{
    std::vector<AttrPair> attrib_list;
    std::vector<std::pair<UString, std::vector<UString> > > my_attrib_list;
    split_attr_pair(ustr, attrib_list);
    for(std::size_t i=0;i<attrib_list.size();i++)
    {
        Attribute attribute;
        attribute.is_optional = false;
        const std::vector<izenelib::util::UString>& attrib_value_list = attrib_list[i].second;
        if(attrib_value_list.size()!=1) continue; //ignore empty value attrib and multi value attribs
        izenelib::util::UString attrib_value = attrib_value_list[0];
        izenelib::util::UString attrib_name = attrib_list[i].first;
        if(attrib_value.length()==0 || attrib_value.length()>30) continue;
        attrib_name.convertString(attribute.name, UString::UTF_8);
        boost::algorithm::trim(attribute.name);
        if(boost::algorithm::ends_with(attribute.name, "_optional"))
        {
            attribute.is_optional = true;
            attribute.name = attribute.name.substr(0, attribute.name.find("_optional"));
        }
        std::vector<std::string> value_list;
        std::string svalue;
        attrib_value.convertString(svalue, izenelib::util::UString::UTF_8);
        boost::algorithm::split(attribute.values, svalue, boost::algorithm::is_any_of("/"));
        for(uint32_t v=0;v<attribute.values.size();v++)
        {
            boost::algorithm::trim(attribute.values[v]);
        }
        if(attribute.name=="容量" && attribute.values.size()==1 && boost::algorithm::ends_with(attribute.values[0], "G"))
        {
            std::string gb_value = attribute.values[0]+"B";
            attribute.values.push_back(gb_value);
        }
        attributes.push_back(attribute);
    }
}
ProductMatcher::term_t ProductMatcher::GetTerm_(const UString& text)
{
    term_t term = izenelib::util::HashFunction<izenelib::util::UString>::generateHash32(text);
    //term_t term = izenelib::util::HashFunction<izenelib::util::UString>::generateHash64(text);
    id_manager_[term] = text;
    return term;
    //term_t tid = 0;
    //if(!aid_manager_->getDocIdByDocName(text, tid))
    //{
        //tid = tid_;
        //++tid_;
        //aid_manager_->Set(text, tid);
    //}
    //return tid;
}
ProductMatcher::term_t ProductMatcher::GetTerm_(const std::string& text)
{
    UString utext(text, UString::UTF_8);
    return GetTerm_(utext);
}

std::string ProductMatcher::GetText_(const TermList& tl)
{
    std::string result;
    for(uint32_t i=0;i<tl.size();i++)
    {
        std::string str = " ";
        term_t term = tl[i];
        UString ustr = id_manager_[term];
        if(ustr.length()>0)
        {
            ustr.convertString(str, UString::UTF_8);
        }
        result+=str;
    }
    return result;
}

void ProductMatcher::GetTerms_(const UString& text, std::vector<term_t>& term_list)
{
    std::vector<UString> text_list;
    AnalyzeChar_(text, text_list);
    term_list.resize(text_list.size());
    for(uint32_t i=0;i<term_list.size();i++)
    {
        term_list[i] = GetTerm_(text_list[i]);
    }
}
void ProductMatcher::GetCRTerms_(const UString& text, std::vector<term_t>& term_list)
{
    std::vector<UString> text_list;
    AnalyzeCR_(text, text_list);
    term_list.resize(text_list.size());
    for(uint32_t i=0;i<term_list.size();i++)
    {
        term_list[i] = GetTerm_(text_list[i]);
    }
}
void ProductMatcher::GetTerms_(const std::string& text, std::vector<term_t>& term_list)
{
    UString utext(text, UString::UTF_8);
    GetTerms_(utext, term_list);
}
void ProductMatcher::GenSuffixes_(const std::vector<term_t>& term_list, Suffixes& suffixes)
{
    suffixes.resize(term_list.size());
    for(uint32_t i=0;i<term_list.size();i++)
    {
        suffixes[i].assign(term_list.begin()+i, term_list.end());
    }
}
void ProductMatcher::GenSuffixes_(const std::string& text, Suffixes& suffixes)
{
    std::vector<term_t> terms;
    GetTerms_(text, terms);
    GenSuffixes_(terms, suffixes);
}
void ProductMatcher::ConstructSuffixTrie_(TrieType& trie)
{
    for(uint32_t i=0;i<category_list_.size();i++)
    {
        if(i%100==0)
        {
            LOG(INFO)<<"scanning category "<<i<<std::endl;
            //LOG(INFO)<<"max tid "<<aid_manager_->getMaxDocId()<<std::endl;
        }
        uint32_t cid = i;
        const Category& category = category_list_[i];
        if(category.is_parent) continue;
        if(category.name.empty()) continue;
        std::vector<std::string> cs_list;
        boost::algorithm::split( cs_list, category.name, boost::algorithm::is_any_of(">") );
        if(cs_list.empty()) continue;
        typedef boost::unordered_map<TermList, std::pair<bool, uint32_t> > AppType;
        AppType app;
        
        for(uint32_t c=0;c<cs_list.size();c++)
        {
            uint32_t depth = cs_list.size()-c;
            std::string cs = cs_list[c];
            std::vector<std::string> words;
            boost::algorithm::split( words, cs, boost::algorithm::is_any_of("/") );
            for(uint32_t i=0;i<words.size();i++)
            {
                std::string w = words[i];
                std::vector<term_t> term_list;
                GetTerms_(w, term_list);
                Suffixes suffixes;
                GenSuffixes_(term_list, suffixes);
                for(uint32_t s=0;s<suffixes.size();s++)
                {
                    CategoryNameApp cn_app;
                    cn_app.cid = cid;
                    cn_app.depth = depth;
                    cn_app.is_complete = false;
                    if(s==0) cn_app.is_complete = true;
                    //trie[suffixes[s]].category_name_apps.push_back(cn_app);
                    if(app.find(suffixes[s])==app.end())
                    {
                        trie[suffixes[s]].category_name_apps.push_back(cn_app);
                    }
                    else
                    {
                        std::pair<bool, uint32_t>& last = app[suffixes[s]];
                        if(! (last.first && !cn_app.is_complete))
                        {
                            last.first = cn_app.is_complete;
                            last.second = cn_app.depth;
                            trie[suffixes[s]].category_name_apps.back() = cn_app;
                        }
                    }
                }
            }
        }
    }
    for(uint32_t i=0;i<products_.size();i++)
    {
        if(i%100000==0)
        {
            LOG(INFO)<<"scanning product "<<i<<std::endl;
            //LOG(INFO)<<"max tid "<<aid_manager_->getMaxDocId()<<std::endl;
        }
        uint32_t pid = i;
        const Product& product = products_[i];
        std::vector<term_t> title_terms;
        GetTerms_(product.stitle, title_terms);
        Suffixes title_suffixes;
        GenSuffixes_(title_terms, title_suffixes);
        for(uint32_t s=0;s<title_suffixes.size();s++)
        {
            SpuTitleApp st_app;
            st_app.spu_id = pid;
            st_app.pstart = s;
            st_app.pend = title_terms.size();
            trie[title_suffixes[s]].spu_title_apps.push_back(st_app);
        }
        const std::vector<Attribute>& attributes = products_[i].attributes;
        for(uint32_t a=0;a<attributes.size();a++)
        {
            const Attribute& attribute = attributes[a];
            for(uint32_t v=0;v<attribute.values.size();v++)
            {
                std::vector<term_t> terms;
                GetTerms_(attribute.values[v], terms);
                AttributeApp a_app;
                a_app.spu_id = pid;
                a_app.attribute_name = attribute.name;
                a_app.is_optional = attribute.is_optional;
                trie[terms].attribute_apps.push_back(a_app);
                //Suffixes suffixes;
                //GenSuffixes_(attribute.values[v], suffixes);
                //for(uint32_t s=0;s<suffixes.size();s++)
                //{
                    //AttributeApp a_app;
                    //a_app.spu_id = pid;
                    //a_app.attribute_name = attribute.name;
                    //a_app.is_optional = attribute.is_optional;
                    //trie[suffixes[s]].attribute_apps.push_back(a_app);
                //}

            }
        }
    }
}

void ProductMatcher::AddKeyword_(const UString& otext)
{
    std::string str;
    otext.convertString(str, UString::UTF_8);
    boost::algorithm::trim(str);
    UString text(str, UString::UTF_8);
    if(text.length()<2) return;
    int value_type = 0;//0=various, 1=all digits, 2=all alphabets
    for(uint32_t u=0;u<text.length();u++)
    {
        int ctype = 0;
        if(text.isDigitChar(u))
        {
            ctype = 1;
        }
        else if(text.isAlphaChar(u))
        {
            ctype = 2;
        }
        if(u==0)
        {
            value_type = ctype;
        }
        else {
            if(value_type==1&&ctype==1)
            {
            }
            else if(value_type==2&&ctype==2)
            {
            }
            else
            {
                value_type = 0;
            }
        }
        if(value_type==0) break;
    }
    if(value_type==2 && text.length()<3) return;
    if(value_type==1 && text.length()<4) return;
    std::vector<term_t> term_list;
    GetTerms_(text, term_list);
    if(term_list.empty()) return;
    if(not_keywords_.find(term_list)!=not_keywords_.end()) return;
    if(term_list.size()==1)
    {
        UString new_text(GetText_(term_list), UString::UTF_8);
        if(new_text.length()<2) return;
    }
    keyword_set_.insert(term_list);
    //std::cout<<"[AK]"<<str;
    //for(uint32_t i=0;i<term_list.size();i++)
    //{
        //std::cout<<","<<term_list[i];
    //}
    //std::cout<<std::endl;
}

void ProductMatcher::ConstructKeywords_()
{
    for(uint32_t i=1;i<category_list_.size();i++)
    {
        if(i%100==0)
        {
            LOG(INFO)<<"keywords scanning category "<<i<<std::endl;
        }
        const Category& category = category_list_[i];
        if(category.is_parent) continue;
        if(category.name.empty()) continue;
        std::vector<std::string> cs_list;
        boost::algorithm::split( cs_list, category.name, boost::algorithm::is_any_of(">") );
        if(cs_list.empty()) continue;
        boost::unordered_set<std::string> app;
        
        for(uint32_t c=0;c<cs_list.size();c++)
        {
            std::string cs = cs_list[c];
            std::vector<std::string> words;
            boost::algorithm::split( words, cs, boost::algorithm::is_any_of("/") );
            for(uint32_t i=0;i<words.size();i++)
            {
                std::string w = words[i];
                UString text(w, UString::UTF_8);
                AddKeyword_(text);
            }
        }
    }
    LOG(INFO)<<"keywords count "<<keyword_set_.size()<<std::endl;
    for(uint32_t i=1;i<products_.size();i++)
    {
        if(i%100000==0)
        {
            LOG(INFO)<<"keywords scanning product "<<i<<std::endl;
        }
        const Product& product = products_[i];
        const std::vector<Attribute>& attributes = product.attributes;
        for(uint32_t a=0;a<attributes.size();a++)
        {
            const Attribute& attribute = attributes[a];
            for(uint32_t v=0;v<attribute.values.size();v++)
            {
                const std::string& svalue = attribute.values[v];
                UString uvalue(svalue, UString::UTF_8);
                AddKeyword_(uvalue);
            }
        }
    }
    LOG(INFO)<<"keywords count "<<keyword_set_.size()<<std::endl;
    for(uint32_t i=0;i<keywords_thirdparty_.size();i++)
    {
        UString uvalue(keywords_thirdparty_[i], UString::UTF_8);
        AddKeyword_(uvalue);
    }
    LOG(INFO)<<"keywords count "<<keyword_set_.size()<<std::endl;
    //for(KeywordSet::const_iterator it = keyword_set_.begin(); it!=keyword_set_.end();it++)
    //{
        //const TermList& tl = *it;
        //std::string text = GetText_(tl);
        //std::cout<<"[FK]"<<text;
        //for(uint32_t i=0;i<tl.size();i++)
        //{
            //std::cout<<","<<tl[i];
        //}
        //std::cout<<std::endl;
    //}
}

void ProductMatcher::ConstructKeywordTrie_(const TrieType& suffix_trie)
{
    std::vector<TermList> keywords;
    for(KeywordSet::const_iterator it = keyword_set_.begin(); it!=keyword_set_.end();it++)
    {
        keywords.push_back(*it);
    }
    std::sort(keywords.begin(), keywords.end());
    //typedef TrieType::const_node_iterator node_iterator;
    //std::stack<node_iterator> stack;
    //stack.push(suffix_trie.node_begin());
    for(uint32_t k=0;k<keywords.size();k++)
    {
        if(k%100000==0)
        {
            LOG(INFO)<<"build keyword "<<k<<std::endl;
        }
        const TermList& keyword = keywords[k];
        //std::string keyword_str = GetText_(keyword);
        //LOG(INFO)<<"keyword "<<keyword_str<<std::endl;
        KeywordTag tag;
        //find keyword in suffix_trie
        for(TrieType::const_iterator it = suffix_trie.lower_bound(keyword);it!=suffix_trie.end();it++)
        {
            const TermList& key = it->first;
            //std::string key_str = GetText_(key);
            const KeywordTag& value = it->second;
            if(StartsWith_(key, keyword))
            {
                bool is_complete = false;
                if(key==keyword) is_complete = true;
                //LOG(INFO)<<"key found "<<key_str<<std::endl;
                tag.Append(value, is_complete);
                //tag+=value;
            }
            else
            {
                //LOG(INFO)<<"key break "<<key_str<<std::endl;
                break;
            }
        }
        tag.Flush();
        trie_[keyword] = tag;
    }
}

