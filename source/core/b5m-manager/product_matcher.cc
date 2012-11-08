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
using namespace sf1r;
using namespace idmlib::sim;
using namespace idmlib::kpe;
using namespace idmlib::util;

#define Malloc(type,n) (type *)malloc((n)*sizeof(type))

//#define B5M_DEBUG

ProductMatcher::ProductMatcher(const std::string& path)
:path_(path), is_open_(false), tid_(1), aid_manager_(NULL), analyzer_(NULL), char_analyzer_(NULL), chars_analyzer_(NULL),
 test_docid_("7bc999f5d10830d0c59487bd48a73cae"),
 left_bracket_("("), right_bracket_(")")
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
        path = path_+"/category_list";
        izenelib::am::ssf::Util<>::Load(path, category_list_);
        LOG(INFO)<<"category list size "<<category_list_.size()<<std::endl;
        path = path_+"/keywords";
        izenelib::am::ssf::Util<>::Load(path, keywords_thirdparty_);
        path = path_+"/category_index";
        izenelib::am::ssf::Util<>::Load(path, category_index_);
        path = path_+"/attrib_id";
        boost::filesystem::create_directories(path);
        aid_manager_ = new AttributeIdManager(path+"/id");
        if(!products_.empty())
        {
            TrieType suffix_trie;
            ConstructSuffixTrie_(suffix_trie);
            KeywordSet keyword_set;
            ConstructKeywords_(keyword_set);
            LOG(INFO)<<"find "<<keyword_set.size()<<" keywords"<<std::endl;
            ConstructKeywordTrie_(suffix_trie, keyword_set, trie_);
        }
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
        is_open_ = true;
    }
    return true;
}

void ProductMatcher::Clear(const std::string& path)
{
    if(!boost::filesystem::exists(path)) return;
    std::vector<std::string> runtime_path;
    runtime_path.push_back("logger");
    runtime_path.push_back("attrib_id");
    runtime_path.push_back("cid_set");
    runtime_path.push_back("products");
    runtime_path.push_back("a2p");
    runtime_path.push_back("category_group");
    runtime_path.push_back("category_keywords");
    runtime_path.push_back("category.txt");
    runtime_path.push_back("cr_result");
    runtime_path.push_back("category_list");
    runtime_path.push_back("keywords");
    runtime_path.push_back("category_index");

    for(uint32_t i=0;i<runtime_path.size();i++)
    {
        boost::filesystem::remove_all(path+"/"+runtime_path[i]);
    }
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
        if(n>=15000) break;
        if(n%100000==0)
        {
            LOG(INFO)<<"Find Product Documents "<<n<<std::endl;
        }
        Document doc;
        izenelib::util::UString oid;
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
                oid = p->second;
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
        PidType piid = products_.size();
        std::string stitle;
        title.convertString(stitle, izenelib::util::UString::UTF_8);
        std::string soid;
        oid.convertString(soid, UString::UTF_8);
        Product product;
        product.spid = soid;
        product.stitle = stitle;
        product.scategory = scategory;
        product.price = price;
        ParseAttributes_(attrib_ustr, product.attributes);
        products_.push_back(product);
        //std::cerr<<"[SPU][Title]"<<stitle<<std::endl;
    }
    std::string path = path_+"/products";
    izenelib::am::ssf::Util<>::Save(path, products_);
    path = path_+"/category_list";
    LOG(INFO)<<"category list size "<<category_list_.size()<<std::endl;
    izenelib::am::ssf::Util<>::Save(path, category_list_);
    path = path_+"/keywords";
    izenelib::am::ssf::Util<>::Save(path, keywords_thirdparty_);
    path = path_+"/category_index";
    izenelib::am::ssf::Util<>::Save(path, category_index_);
    
    return true;
}


bool ProductMatcher::DoMatch(const std::string& scd_path)
{
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
            if(n%100000==0)
            {
                LOG(INFO)<<"Find Offer Documents "<<n<<std::endl;
            }
            SCDDoc& scddoc = *(*doc_iter);
            SCDDoc::iterator p = scddoc.begin();
            Document doc;
            for(; p!=scddoc.end(); ++p)
            {
                const std::string& property_name = p->first;
                doc.property(property_name) = p->second;
            }
            Process(doc);
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
    return true;
}

bool ProductMatcher::Process(Document& doc)
{
    izenelib::util::UString title;
    izenelib::util::UString category;
    doc.getProperty("Category", category);
    doc.getProperty("Title", title);
    
    if(title.length()==0)
    {
        return false;
    }
    TermList term_list;
    GetTerms_(title, term_list);

    return true;
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
        if(boost::algorithm::ends_with(attribute.name, "_optional"))
        {
            attribute.is_optional = true;
            attribute.name = attribute.name.substr(0, attribute.name.find("_optional"));
        }
        std::vector<std::string> value_list;
        std::string svalue;
        attrib_value.convertString(svalue, izenelib::util::UString::UTF_8);
        boost::algorithm::split(attribute.values, svalue, boost::algorithm::is_any_of("/"));
        attributes.push_back(attribute);
    }
}
ProductMatcher::term_t ProductMatcher::GetTerm_(const UString& text)
{
    term_t tid = 0;
    if(!aid_manager_->getDocIdByDocName(text, tid))
    {
        tid = tid_;
        ++tid_;
        aid_manager_->Set(text, tid);
    }
    return tid;
}

std::string ProductMatcher::GetText_(const TermList& tl)
{
    std::string result;
    for(uint32_t i=0;i<tl.size();i++)
    {
        term_t term = tl[i];
        UString ustr;
        aid_manager_->getDocNameByDocId(term, ustr);
        std::string str;
        ustr.convertString(str, UString::UTF_8);
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
        if(i%10==0)
        {
            LOG(INFO)<<"scanning category "<<i<<std::endl;
            LOG(INFO)<<"max tid "<<aid_manager_->getMaxDocId()<<std::endl;
        }
        uint32_t cid = i+1;
        const Category& category = category_list_[i];
        if(category.is_parent) continue;
        if(category.name.empty()) continue;
        std::vector<std::string> cs_list;
        boost::algorithm::split( cs_list, category.name, boost::algorithm::is_any_of(">") );
        if(cs_list.empty()) continue;
        boost::unordered_set<std::string> app;
        
        for(uint32_t c=0;c<cs_list.size();c++)
        {
            uint32_t depth = c+1;
            std::string cs = cs_list[c];
            std::vector<std::string> words;
            boost::algorithm::split( words, cs, boost::algorithm::is_any_of("/") );
            for(uint32_t i=0;i<words.size();i++)
            {
                std::string w = words[i];
                CategoryNameApp cn_app;
                cn_app.cid = cid;
                cn_app.depth = depth;
                std::vector<term_t> term_list;
                GetTerms_(w, term_list);
                Suffixes suffixes;
                GenSuffixes_(term_list, suffixes);
                for(uint32_t s=0;s<suffixes.size();s++)
                {
                    //LOG(INFO)<<"inserting trie "<<w<<" vector size "<<suffixes[s].size()<<std::endl;
                    //KeywordTag kt;
                    //kt.category_name_apps.push_back(cn_app);
                    //trie.insert(std::make_pair(suffixes[s], kt));
                    trie[suffixes[s]].category_name_apps.push_back(cn_app);
                }
            }
        }
    }
    for(uint32_t i=0;i<products_.size();i++)
    {
        if(i%1000==0)
        {
            LOG(INFO)<<"scanning product "<<i<<std::endl;
            LOG(INFO)<<"max tid "<<aid_manager_->getMaxDocId()<<std::endl;
        }
        uint32_t pid = i+1;
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
                Suffixes suffixes;
                GenSuffixes_(attribute.values[v], suffixes);
                for(uint32_t s=0;s<suffixes.size();s++)
                {
                    AttributeApp a_app;
                    a_app.spu_id = pid;
                    a_app.attribute_name = attribute.name;
                    a_app.is_optional = attribute.is_optional;
                    trie[suffixes[s]].attribute_apps.push_back(a_app);
                }

            }
        }
    }
}

void ProductMatcher::ConstructKeywords_(boost::unordered_set<TermList>& keywords)
{
    for(uint32_t i=1;i<category_list_.size();i++)
    {
        if(i%1000==0)
        {
            LOG(INFO)<<"scanning category "<<i<<std::endl;
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
                std::vector<term_t> term_list;
                GetTerms_(w, term_list);
                if(!term_list.empty()) keywords.insert(term_list);
            }
        }
    }
    for(uint32_t i=1;i<products_.size();i++)
    {
        if(i%1000==0)
        {
            LOG(INFO)<<"scanning product "<<i<<std::endl;
        }
        const Product& product = products_[i];
        const std::vector<Attribute>& attributes = product.attributes;
        for(uint32_t a=0;a<attributes.size();a++)
        {
            const Attribute& attribute = attributes[a];
            for(uint32_t v=0;v<attribute.values.size();v++)
            {
                std::vector<term_t> term_list;
                GetTerms_(attribute.values[v], term_list);
                if(!term_list.empty()) keywords.insert(term_list);
            }
        }
    }
    for(uint32_t i=0;i<keywords_thirdparty_.size();i++)
    {
        std::vector<term_t> term_list;
        GetTerms_(keywords_thirdparty_[i], term_list);
        if(!term_list.empty()) keywords.insert(term_list);
    }
}

void ProductMatcher::ConstructKeywordTrie_(const TrieType& suffix_trie, const KeywordSet& keyword_set, TrieType& trie)
{
    std::vector<TermList> keywords;
    for(KeywordSet::const_iterator it = keyword_set.begin(); it!=keyword_set.end();it++)
    {
        keywords.push_back(*it);
    }
    std::sort(keywords.begin(), keywords.end());
    typedef TrieType::const_node_iterator node_iterator;
    std::stack<node_iterator> stack;
    stack.push(suffix_trie.node_begin());
    for(uint32_t k=0;k<keywords.size();k++)
    {
        if(k%10==0)
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
                //LOG(INFO)<<"key found "<<key_str<<std::endl;
                tag+=value;
            }
            else
            {
                //LOG(INFO)<<"key break "<<key_str<<std::endl;
                break;
            }
        }
        trie[keyword] = tag;
    }
}

