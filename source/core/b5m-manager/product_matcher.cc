#include "product_matcher.h"
#include <util/hashFunction.h>
#include <common/ScdParser.h>
#include <common/ScdWriter.h>
#include <document-manager/JsonDocument.h>
#include <mining-manager/util/split_ustr.h>
#include <product-manager/product_price.h>
#include <boost/unordered_set.hpp>
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int.hpp>
#include <boost/serialization/hash_map.hpp>
#include <boost/dynamic_bitset.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <algorithm>
#include <cmath>
#include <idmlib/util/svm.h>
#include <util/functional.h>
#include <util/ClockTimer.h>
#include <3rdparty/json/json.h>
using namespace sf1r;
using namespace idmlib::sim;
using namespace idmlib::kpe;
using namespace idmlib::util;
namespace bfs = boost::filesystem;


//#define B5M_DEBUG

const std::string ProductMatcher::AVERSION("20130710000000");

ProductMatcher::KeywordTag::KeywordTag():type_app(0), kweight(0.0), ngram(1)
{
}

void ProductMatcher::KeywordTag::Flush()
{
    std::sort(category_name_apps.begin(), category_name_apps.end());
    std::vector<CategoryNameApp> new_category_name_apps;
    new_category_name_apps.reserve(category_name_apps.size());
    for(uint32_t i=0;i<category_name_apps.size();i++)
    {
        const CategoryNameApp& app = category_name_apps[i];
        if(new_category_name_apps.empty())
        {
            new_category_name_apps.push_back(app);
            continue;
        }
        CategoryNameApp& last_app = new_category_name_apps.back();
        if(app.cid==last_app.cid)
        {
            if(app.is_complete&&!last_app.is_complete)
            {
                last_app = app;
            }
            else if(app.is_complete==last_app.is_complete&&app.depth<last_app.depth)
            {
                last_app = app;
            }
        }
        else
        {
            new_category_name_apps.push_back(app);
        }
    }
    std::swap(new_category_name_apps, category_name_apps);
    //SortAndUnique(category_name_apps);
    SortAndUnique(attribute_apps);
    //SortAndUnique(spu_title_apps);
}
void ProductMatcher::KeywordTag::Append(const KeywordTag& another, bool is_complete)
{
    if(is_complete)
    {
        for(uint32_t i=0;i<another.category_name_apps.size();i++)
        {
            CategoryNameApp aapp = another.category_name_apps[i];
            if(!is_complete) aapp.is_complete = false;
            category_name_apps.push_back(aapp);
        }
    }
    if(is_complete)
    {
        attribute_apps.insert(attribute_apps.end(), another.attribute_apps.begin(), another.attribute_apps.end());
    }
    //spu_title_apps.insert(spu_title_apps.end(), another.spu_title_apps.begin(), another.spu_title_apps.end());
}
void ProductMatcher::KeywordTag::PositionMerge(const Position& pos)
{
    bool combine = false;
    for(uint32_t i=0;i<positions.size();i++)
    {
        if(positions[i].Combine(pos))
        {
            combine = true;
            break;
        }
    }
    if(!combine)
    {
        positions.push_back(pos);
    }
}
bool ProductMatcher::KeywordTag::Combine(const KeywordTag& another)
{
    //check if positions were overlapped
    //for(uint32_t i=0;i<another.positions.size();i++)
    //{
        //const Position& ap = another.positions[i];
        ////std::cerr<<"ap "<<ap.first<<","<<ap.second<<std::endl;
        //for(uint32_t j=0;j<positions.size();j++)
        //{
            //const Position& p = positions[j];
            ////std::cerr<<"p "<<p.first<<","<<p.second<<std::endl;
            //bool _overlapped = true;
            //if( ap.first>=p.second || p.first>=ap.second) _overlapped = false;
            //if(_overlapped)
            //{
                //return false;
            //}
        //}
    //}
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
    //i = 0;
    //j = 0;
    //while(i<spu_title_apps.size()&&j<another.spu_title_apps.size())
    //{
        //SpuTitleApp& app = spu_title_apps[i];
        //const SpuTitleApp& aapp = another.spu_title_apps[j];
        //if(app.spu_id==aapp.spu_id)
        //{
            //app.pstart = std::min(app.pstart, aapp.pstart);
            //app.pend = std::max(app.pend, aapp.pend);
            //++i;
            //++j;
        //}
        //else if(app<aapp)
        //{
            //app.spu_id = 0;
            //++i;
        //}
        //else
        //{
            //++j;
        //}
    //}
    //while(i<spu_title_apps.size())
    //{
        //SpuTitleApp& app = spu_title_apps[i];
        //app.spu_id = 0;
        //++i;
    //}
    ++ngram;
    positions.insert(positions.end(), another.positions.begin(), another.positions.end());
    return true;
}

bool ProductMatcher::KeywordTag::IsAttribSynonym(const KeywordTag& another) const
{
    bool result = false;
    for(uint32_t i=0;i<attribute_apps.size();i++)
    {
        const AttributeApp& ai = attribute_apps[i];
        for(uint32_t j=0;j<another.attribute_apps.size();j++)
        {
            const AttributeApp& aj = another.attribute_apps[j];
            if(ai==aj)
            {
                result = true;
                break;
            }
        }
        if(result) break;
    }
    return result;
}
bool ProductMatcher::KeywordTag::IsBrand() const
{
    for(uint32_t i=0;i<attribute_apps.size();i++)
    {
        const AttributeApp& ai = attribute_apps[i];
        if(ai.attribute_name=="品牌") return true;
    }
    return false;
}
bool ProductMatcher::KeywordTag::IsModel() const
{
    for(uint32_t i=0;i<attribute_apps.size();i++)
    {
        const AttributeApp& ai = attribute_apps[i];
        if(ai.attribute_name=="型号") return true;
    }
    return false;
}

ProductMatcher::ProductMatcher()
:is_open_(false),
 use_price_sim_(true), matcher_only_(false), category_max_depth_(0), use_ngram_(false),
 aid_manager_(NULL), analyzer_(NULL), char_analyzer_(NULL), chars_analyzer_(NULL),
 test_docid_("7bc999f5d10830d0c59487bd48a73cae"),
 left_bracket_("("), right_bracket_(")"), place_holder_("__PLACE_HOLDER__"), blank_(" "),
 left_bracket_term_(0), right_bracket_term_(0), place_holder_term_(0),
 type_regex_("[a-zA-Z\\d\\-]{4,}"), vol_regex_("^(8|16|32|64)gb?$"),
 book_category_("书籍/杂志/报纸")
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

bool ProductMatcher::Open(const std::string& kpath)
{
    if(!is_open_)
    {
        path_ = kpath;
        try
        {
            Init_();
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
            path = path_+"/cid_to_pids";
            izenelib::am::ssf::Util<>::Load(path, cid_to_pids_);
            path = path_+"/product_index";
            izenelib::am::ssf::Util<>::Load(path, product_index_);
            path = path_+"/keyword_trie";
            izenelib::am::ssf::Util<>::Load(path, trie_);
            LOG(INFO)<<"trie size "<<trie_.size()<<std::endl;
            //path = path_+"/fuzzy_trie";
            //izenelib::am::ssf::Util<>::Load(path, ftrie_);
            //LOG(INFO)<<"fuzzy size "<<ftrie_.size()<<std::endl;
            path = path_+"/term_index";
            std::map<cid_t, TermIndex> tmap;
            izenelib::am::ssf::Util<>::Load(path, tmap);
            term_index_map_.insert(tmap.begin(), tmap.end());
            LOG(INFO)<<"term_index map size "<<term_index_map_.size()<<std::endl;
            path = path_+"/back2front";
            std::map<std::string, std::string> b2f_map;
            izenelib::am::ssf::Util<>::Load(path, b2f_map);
            back2front_.insert(b2f_map.begin(), b2f_map.end());
            LOG(INFO)<<"back2front size "<<back2front_.size()<<std::endl;
            path = path_+"/book_category";
            if(boost::filesystem::exists(path))
            {
                izenelib::am::ssf::Util<>::Load(path, book_category_);
            }
            path = path_+"/feature_vector";
            izenelib::am::ssf::Util<>::LoadOne(path, feature_vectors_);

            for(uint32_t i=0;i<feature_vectors_.size();i++)
            {
                std::string category = category_list_[i].name;
//                LOG(INFO)<<"i: "<<i<<" cid: "<<category_list_[i].cid<<endl;
                cid_t cid = GetLevelCid_(category, 1);
                first_level_category_[cid] = 1;
//                LOG(INFO)<<"first level cid: "<<cid<<" name: "<<category_list_[cid].name<<endl;
            }
            //path = path_+"/nf";
            //std::map<uint64_t, uint32_t> nf_map;
            //izenelib::am::ssf::Util<>::Load(path, nf_map);
            //nf_.insert(nf_map.begin(), nf_map.end());
            //LOG(INFO)<<"nf size "<<nf_.size()<<std::endl;
            std::vector<std::pair<size_t, string> > synonym_pairs;
            path = path_+"/synonym_map";
            izenelib::am::ssf::Util<>::Load(path, synonym_pairs);
            for (size_t i = 0; i < synonym_pairs.size(); ++i)
                synonym_map_.insert(std::make_pair(synonym_pairs[i].second, synonym_pairs[i].first));

            std::vector<string> tmp_sets;
            path = path_+"/synonym_dict";
            izenelib::am::ssf::Util<>::Load(path, tmp_sets);
            for (size_t i = 0; i < tmp_sets.size(); ++i)
            {
                std::vector<string> tmp_set;
                boost::algorithm::split(tmp_set, tmp_sets[i], boost::algorithm::is_any_of("/"));
                synonym_dict_.push_back(tmp_set);
            }

            LOG(INFO)<<"synonym map size "<<synonym_pairs.size();
            LOG(INFO)<<"synonym dict size "<<synonym_dict_.size();
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

bool ProductMatcher::GetSynonymSet(const UString& pattern, std::vector<UString>& synonym_set, int& setid)
{
    if (synonym_map_.empty() || synonym_dict_.empty())
    {
        LOG(INFO)<<"synonym dict is empty!";
        return false;
    }
    string st;
    pattern.convertString(st, UString::UTF_8);
    if (synonym_map_.find(st) == synonym_map_.end())
    {
        return false;
    }
    else
    {
        setid = synonym_map_[st];
        for (size_t i = 0; i < synonym_dict_[setid].size(); ++i)
        {
            UString ust;
            ust.assign(synonym_dict_[setid][i], UString::UTF_8);
            synonym_set.push_back(ust);
        }
    }
    return true;
}
//void ProductMatcher::Clear(const std::string& path, int omode)
//{
    //int mode = omode;
    //std::string version = GetVersion(path);
    //if(version!=VERSION)
    //{
        //mode = 3;
    //}
    //if(mode>=2)
    //{
        //B5MHelper::PrepareEmptyDir(path);
    //}
    //else if(mode>0)
    //{
        //std::string bdb_path = path+"/bdb";
        //B5MHelper::PrepareEmptyDir(bdb_path);
        ////if(mode>=2)
        ////{
            ////std::string odb_path = path+"/odb";
            ////B5MHelper::PrepareEmptyDir(odb_path);
        ////}
    //}
//}
std::string ProductMatcher::GetVersion(const std::string& path)
{
    std::string version_file = path+"/VERSION";
    std::string version;
    std::ifstream ifs(version_file.c_str());
    getline(ifs, version);
    boost::algorithm::trim(version);
    return version;
}
std::string ProductMatcher::GetAVersion(const std::string& path)
{
    std::string version_file = path+"/AVERSION";
    std::string version;
    std::ifstream ifs(version_file.c_str());
    getline(ifs, version);
    boost::algorithm::trim(version);
    return version;
}
std::string ProductMatcher::GetRVersion(const std::string& path)
{
    std::string version_file = path+"/RVERSION";
    std::string version;
    std::ifstream ifs(version_file.c_str());
    getline(ifs, version);
    boost::algorithm::trim(version);
    return version;
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
void ProductMatcher::CategoryDepthCut(std::string& category, uint16_t d)
{
    if(d==0) return;
    if(category.empty()) return;
    std::vector<std::string> vec;
    boost::algorithm::split(vec, category, boost::algorithm::is_any_of(">"));
    if(vec.size()>d)
    {
        category="";
        for(uint16_t i=0;i<d;i++)
        {
            if(!category.empty()) category+=">";
            category+=vec[i];
        }
    }
}

bool ProductMatcher::GetFrontendCategory(UString& backend, UString& frontend) const
{
    if(back2front_.empty()) 
    {
        frontend = backend;
        return true;
    }
    std::string b;
    backend.convertString(b, UString::UTF_8);
    std::string ob = b;//original
    while(!b.empty())
    {
        Back2Front::const_iterator it = back2front_.find(b);
        if(it!=back2front_.end())
        {
            std::string sfront = it->second;
            CategoryDepthCut(sfront, category_max_depth_);
            frontend = UString(sfront, UString::UTF_8);
            break;
        }
        std::size_t pos = b.find_last_of('>');
        if(pos==std::string::npos) b.clear();
        else
        {
            b = b.substr(0, pos);
        }
    }
    if(frontend.length()>0 && b!=ob)
    {
        backend = UString(b, UString::UTF_8);
    }
    if(b==ob) return true;
    return false;
}

void ProductMatcher::SetIndexDone_(const std::string& path, bool b)
{
    static const std::string file(path+"/index.done");
    if(b)
    {
        std::ofstream ofs(file.c_str());
        ofs<<"a"<<std::endl;
        ofs.close();
    }
    else
    {
        boost::filesystem::remove_all(file);
    }
}
bool ProductMatcher::IsIndexDone() const
{
    return trie_.size()>0;
}

bool ProductMatcher::IsIndexDone_(const std::string& path)
{
    static const std::string file(path+"/index.done");
    return boost::filesystem::exists(file);
}
void ProductMatcher::Init_()
{
    boost::filesystem::create_directories(path_);
    //if(analyzer_==NULL)
    //{
        //idmlib::util::IDMAnalyzerConfig aconfig = idmlib::util::IDMAnalyzerConfig::GetCommonConfig("",cma_path_, "");
        //aconfig.symbol = true;
        //analyzer_ = new idmlib::util::IDMAnalyzer(aconfig);
    //}
    //if(char_analyzer_==NULL)
    //{
        //idmlib::util::IDMAnalyzerConfig cconfig = idmlib::util::IDMAnalyzerConfig::GetCommonConfig("","", "");
        //cconfig.symbol = true;
        //char_analyzer_ = new idmlib::util::IDMAnalyzer(cconfig);
    //}
    if(chars_analyzer_==NULL)
    {
        idmlib::util::IDMAnalyzerConfig csconfig = idmlib::util::IDMAnalyzerConfig::GetCommonConfig("","", "");
        csconfig.symbol = true;
        chars_analyzer_ = new idmlib::util::IDMAnalyzer(csconfig);
    }
    left_bracket_term_ = GetTerm_(left_bracket_);
    right_bracket_term_ = GetTerm_(right_bracket_);
    place_holder_term_ = GetTerm_(place_holder_);
    blank_term_ = GetTerm_(blank_);
    //symbol_terms_.insert(blank_term_);
    //symbol_terms_.insert(left_bracket_term_);
    //symbol_terms_.insert(right_bracket_term_);

    std::vector<std::string> connect_symbol_strs;
    connect_symbol_strs.push_back("-");
    connect_symbol_strs.push_back("/");
    //connect_symbol_strs.push_back("|");
    connect_symbol_strs.push_back(blank_);

    for(uint32_t i=0;i<connect_symbol_strs.size();i++)
    {
        connect_symbols_.insert(GetTerm_(connect_symbol_strs[i]));
    }
    std::vector<std::string> text_symbol_strs;
    text_symbol_strs.push_back(".");
    text_symbol_strs.push_back("+");
    for(uint32_t i=0;i<text_symbol_strs.size();i++)
    {
        text_symbols_.insert(GetTerm_(text_symbol_strs[i]));
    }
    products_.clear();
    keywords_thirdparty_.clear();
    not_keywords_.clear();
    category_list_.clear();
    cid_to_pids_.clear();
    category_index_.clear();
    product_index_.clear();
    keyword_set_.clear();
    trie_.clear();
    //ftrie_.clear();
    term_index_map_.clear();
    back2front_.clear();
    feature_vectors_.clear();
    //nf_.clear();

}
bool ProductMatcher::Index(const std::string& kpath, const std::string& scd_path, int omode)
{
    path_ = kpath;
    if(!boost::filesystem::exists(kpath))
    {
        boost::filesystem::create_directories(kpath);
    }
    std::string rversion = GetVersion(scd_path);
    LOG(INFO)<<"rversion "<<rversion<<std::endl;
    LOG(INFO)<<"aversion "<<AVERSION<<std::endl;
    std::string erversion = GetRVersion(kpath);
    std::string eaversion = GetAVersion(kpath);
    LOG(INFO)<<"erversion "<<erversion<<std::endl;
    LOG(INFO)<<"eaversion "<<eaversion<<std::endl;
    int mode = omode;
    if(AVERSION>eaversion)
    {
        if(mode<2) mode = 2;
    }
    if(mode<2) //not re-training, try open
    {
        if(!Open(path_))
        {
            mode = 2;
            Init_();
        }
        if(IsIndexDone())
        {
            std::cout<<"product trained at "<<path_<<std::endl;
            Init_();
            return true;
        }
    }
    LOG(INFO)<<"mode "<<mode<<std::endl;
    if(mode==3)
    {
        B5MHelper::PrepareEmptyDir(kpath);
    }
    else if(mode==2)
    {
        SetIndexDone_(kpath, false);
        //std::string bdb_path = kpath+"/bdb";
        //B5MHelper::PrepareEmptyDir(bdb_path);
    }
    SetIndexDone_(kpath, false);
    Init_();
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
    std::string back2front_file= scd_path+"/back2front";
    std::map<std::string, std::string> b2f_map;
    if(boost::filesystem::exists(back2front_file))
    {
        std::ifstream ifs(back2front_file.c_str());
        std::string line;
        while( getline(ifs, line))
        {
            boost::algorithm::trim(line);
            std::vector<std::string> vec;
            boost::algorithm::split(vec, line, boost::algorithm::is_any_of(","));
            if(vec.size()!=2) continue;
            std::pair<std::string, std::string> value(vec[0], vec[1]);
            b2f_map.insert(value);
            back2front_.insert(value);
        }
        ifs.close();
    }
    std::string bookcategory_file= scd_path+"/book_category";
    if(boost::filesystem::exists(bookcategory_file))
    {
        std::ifstream ifs(bookcategory_file.c_str());
        std::string line;
        if( getline(ifs, line))
        {
            boost::algorithm::trim(line);
            if(!line.empty())
            {
                book_category_ = line;
                LOG(INFO)<<"book category set to "<<book_category_<<std::endl;
            }
        }
        ifs.close();
    }
    std::string spu_scd = scd_path+"/SPU.SCD";
    if(!boost::filesystem::exists(spu_scd))
    {
        std::cerr<<"SPU.SCD not exists"<<std::endl;
        return false;
    }
    products_.resize(1);
    category_list_.resize(1);
    {
        std::string category_path = scd_path+"/category";
        std::string line;
        std::ifstream ifs(category_path.c_str());
        while(getline(ifs, line))
        {
            boost::algorithm::trim(line);
            std::vector<std::string> vec;
            boost::algorithm::split(vec, line, boost::algorithm::is_any_of(","));
            if(vec.size()<1) continue;
            uint32_t cid = category_list_.size();
            const std::string& scategory = vec[0];
            if(scategory.empty()) continue;
            //ignore book category
            if(boost::algorithm::starts_with(scategory, book_category_))
            {
                continue;
            }
            //const std::string& tag = vec[1];
            Category c;
            c.name = scategory;
            c.cid = cid;
            c.parent_cid = 0;
            c.is_parent = false;
            c.has_spu = false;
            std::set<std::string> akeywords;
            std::set<std::string> rkeywords;
            for(uint32_t i=1;i<vec.size();i++)
            {
                std::string keyword = vec[i];
                bool remove = false;
                if(keyword.empty()) continue;
                if(keyword[0]=='+')
                {
                    keyword = keyword.substr(1);
                }
                else if(keyword[0]=='-')
                {
                    keyword = keyword.substr(1);
                    remove = true;
                }
                if(keyword.empty()) continue;
                if(!remove)
                {
                    akeywords.insert(keyword);
                }
                else
                {
                    rkeywords.insert(keyword);
                }
            }
            std::vector<std::string> cs_list;
            boost::algorithm::split( cs_list, c.name, boost::algorithm::is_any_of(">") );
            c.depth=cs_list.size();
            if(cs_list.empty()) continue;
            std::vector<std::string> keywords_vec;
            boost::algorithm::split( keywords_vec, cs_list.back(), boost::algorithm::is_any_of("/") );
            for(uint32_t i=0;i<keywords_vec.size();i++)
            {
                akeywords.insert(keywords_vec[i]);
            }
            for(std::set<std::string>::const_iterator it = rkeywords.begin();it!=rkeywords.end();it++)
            {
                akeywords.erase(*it);
            }
            for(std::set<std::string>::const_iterator it = akeywords.begin();it!=akeywords.end();it++)
            {
                UString uc(*it, UString::UTF_8);
                if(uc.length()<=1) continue;
                c.keywords.push_back(*it);
            }
            if(c.depth>1)
            {
                std::string parent_name;
                for(uint32_t i=0;i<c.depth-1;i++)
                {
                    if(!parent_name.empty())
                    {
                        parent_name+=">";
                    }
                    parent_name+=cs_list[i];
                }
                c.parent_cid = category_index_[parent_name];
                category_list_[c.parent_cid].is_parent = true;
                //std::cerr<<"cid "<<c.cid<<std::endl;
                //std::cerr<<"parent cid "<<c.parent_cid<<std::endl;
            }
            category_list_.push_back(c);
#ifdef B5M_DEBUG
            std::cerr<<"add category "<<c.name<<std::endl;
#endif
            category_index_[c.name] = cid;
        }
        ifs.close();
    }
    //std::string bdb_path = path_+"/bdb";
    //BrandDb bdb(bdb_path);
    //bdb.open();
    ScdParser parser(izenelib::util::UString::UTF_8);
    parser.load(spu_scd);
    uint32_t n=0;
    size_t synonym_dict_size = 0;
    std::vector<std::pair<size_t, string> > synonym_pairs;
    std::vector<string> term_set;

    for( ScdParser::iterator doc_iter = parser.begin();
      doc_iter!= parser.end(); ++doc_iter, ++n)
    {
        //if(n>=15000) break;
        if(n%100000==0)
        {
            LOG(INFO)<<"Find Product Documents "<<n<<std::endl;
        }
        Document doc;
        Document::doc_prop_value_strtype pid;
        Document::doc_prop_value_strtype title;
        Document::doc_prop_value_strtype category;
        Document::doc_prop_value_strtype attrib_ustr;
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
        uint32_t cid = 0;
        std::string scategory = propstr_to_str(category);
        CategoryIndex::iterator cit = category_index_.find(scategory);
        if(cit==category_index_.end())
        {
            continue;
        }
        else
        {
            cid = cit->second;
            Category& category = category_list_[cid];
            //if(category.is_parent) continue;
            category.has_spu = true;
        }
        ProductPrice price;
        Document::doc_prop_value_strtype uprice;
        if(doc.getProperty("Price", uprice))
        {
            price.Parse(propstr_to_ustr(uprice));
        }
        std::string stitle = propstr_to_str(title);
        std::string spid = propstr_to_str(pid);
        uint128_t ipid = B5MHelper::StringToUint128(spid);
        std::string sattribute = propstr_to_str(attrib_ustr);
        Product product;
        product.spid = spid;
        product.stitle = stitle;
        product.scategory = scategory;
        product.cid = cid;
        product.price = price;
        ParseAttributes(propstr_to_ustr(attrib_ustr), product.attributes);

        for (size_t i = 0; i < product.attributes.size(); ++i)
            if (product.attributes[i].name == "品牌")
            {
                if (product.attributes[i].values.empty() || product.attributes[i].values.size() < 2) continue;

                size_t count = 0;
                size_t tmp_id = 0;
                string lower_string;
                for (size_t j = 0; j < product.attributes[i].values.size(); ++j)
                {
                    lower_string = product.attributes[i].values[j];
                    boost::to_lower(lower_string);
                    if (synonym_map_.find(lower_string)!=synonym_map_.end())//term is not in synonym map
                    {
                        ++count;
                        tmp_id = synonym_map_[lower_string];
                    }
                }
                if (!count)//new synonym set
                {
                    string st;
                    for (size_t j = 0; j < product.attributes[i].values.size(); ++j)
                    {
                        lower_string = product.attributes[i].values[j];
                        boost::to_lower(lower_string);
                        synonym_map_.insert(std::make_pair(lower_string, synonym_dict_size));
                        synonym_pairs.push_back(std::make_pair(synonym_dict_size, lower_string));
                        st += (lower_string + '/');
                    }
                    term_set.push_back(st);
                    ++synonym_dict_size;
                }
                else if (count < product.attributes[i].values.size())//add term in synonym set
                {
                    for (size_t j = 0; j < product.attributes[i].values.size(); ++j)
                    {
                        lower_string = product.attributes[i].values[j];
                        boost::to_lower(lower_string);
                        if (synonym_map_.find(lower_string) == synonym_map_.end())
                        {
                            synonym_map_.insert(std::make_pair(lower_string, tmp_id));
                            synonym_pairs.push_back(std::make_pair(tmp_id, lower_string));
                            term_set[tmp_id] += lower_string + '/';
                        }
                    }
                }
            }
        if(product.attributes.size()<2) continue;
        Document::doc_prop_value_strtype dattribute_ustr;
        doc.getProperty("DAttribute", dattribute_ustr);
        Document::doc_prop_value_strtype display_attribute_str;
        doc.getProperty("DisplayAttribute", display_attribute_str);
        product.display_attributes = propstr_to_ustr(display_attribute_str);
        Document::doc_prop_value_strtype filter_attribute_str;
        doc.getProperty("FilterAttribute", filter_attribute_str);
        product.filter_attributes = propstr_to_ustr(filter_attribute_str);
        //if(!filter_attribute_ustr.empty())
        //{
            //ParseAttributes(filter_attribute_ustr, product.dattributes);
        //}
        //else if(!display_attribute_ustr.empty())
        //{
            //ParseAttributes(display_attribute_ustr, product.dattributes);
        //}
        if(!dattribute_ustr.empty())
        {
            ParseAttributes(propstr_to_ustr(dattribute_ustr), product.dattributes);
            MergeAttributes(product.dattributes, product.attributes);
        }
        product.tweight = 0.0;
        product.aweight = 0.0;
        //std::cerr<<"[SPU][Title]"<<stitle<<std::endl;
        boost::unordered_set<std::string> attribute_value_app;
        bool invalid_attribute = false;
        for(uint32_t i=0;i<product.attributes.size();i++)
        {
            const Attribute& attrib = product.attributes[i];
            for(uint32_t a=0;a<attrib.values.size();a++)
            {
                std::string v = attrib.values[a];
                boost::algorithm::to_lower(v);
                if(attribute_value_app.find(v)!=attribute_value_app.end())
                {
                    invalid_attribute = true;
                    break;
                }
                attribute_value_app.insert(v);
            }
            if(invalid_attribute) break;
        }
        if(invalid_attribute)
        {
//#ifdef B5M_DEBUG
//#endif
            LOG(INFO)<<"invalid SPU attribute "<<spid<<","<<sattribute<<std::endl;
            continue;
        }
        for(uint32_t i=0;i<product.attributes.size();i++)
        {
            if(product.attributes[i].is_optional)
            {
                //product.aweight+=0.5;
            }
            else if(product.attributes[i].name=="型号")
            {
                product.aweight+=1.5;
            }
            else
            {
                product.aweight+=1.0;
            }

        }
        //Document::doc_prop_value_strtype brand;
        std::string sbrand;
        for(uint32_t i=0;i<product.attributes.size();i++)
        {
            if(product.attributes[i].name=="品牌")
            {
                sbrand = product.attributes[i].GetValue();
                //brand = str_to_propstr(sbrand, UString::UTF_8);
                break;
            }
        }
        //if(!brand.empty())
        //{
            //BrandDb::BidType bid = bdb.set(ipid, brand);
            //bdb.set_source(brand, bid);
        //}
        product.sbrand = sbrand;
        uint32_t spu_id = products_.size();
        products_.push_back(product);
        product_index_[spid] = spu_id;
    }
    //bdb.flush();
    //add virtual spus
    for(uint32_t i=1;i<category_list_.size();i++)
    {
        const Category& c = category_list_[i];
        if(!c.has_spu)
        {
            Product p;
            p.scategory = c.name;
            p.cid = i;
            products_.push_back(p);
        }
    }
    IndexFuzzy_();
    cid_to_pids_.resize(category_list_.size());
    for(uint32_t spu_id=1;spu_id<products_.size();spu_id++)
    {
        Product& p = products_[spu_id];
        string_similarity_.Convert(p.stitle, p.title_obj);
        cid_to_pids_[p.cid].push_back(spu_id);
    }
    if(!products_.empty())
    {
        TrieType suffix_trie;
        ConstructSuffixTrie_(suffix_trie);
        ConstructKeywords_();
        LOG(INFO)<<"find "<<keyword_set_.size()<<" keywords"<<std::endl;
        ConstructKeywordTrie_(suffix_trie);
    }
    std::string offer_scd = scd_path+"/OFFER.SCD";
    if(boost::filesystem::exists(offer_scd))
    {
        IndexOffer_(offer_scd);
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
    path = path_+"/cid_to_pids";
    izenelib::am::ssf::Util<>::Save(path, cid_to_pids_);
    path = path_+"/product_index";
    izenelib::am::ssf::Util<>::Save(path, product_index_);
    path = path_+"/keyword_trie";
    izenelib::am::ssf::Util<>::Save(path, trie_);
    //path = path_+"/fuzzy_trie";
    //izenelib::am::ssf::Util<>::Save(path, ftrie_);
    path = path_+"/term_index";
    std::map<cid_t, TermIndex> tmap(term_index_map_.begin(), term_index_map_.end());
    izenelib::am::ssf::Util<>::Save(path, tmap);
    path = path_+"/back2front";
    izenelib::am::ssf::Util<>::Save(path, b2f_map);
    path = path_+"/book_category";
    izenelib::am::ssf::Util<>::Save(path, book_category_);
    path = path_+"/feature_vector";
    izenelib::am::ssf::Util<>::SaveOne(path, feature_vectors_);
    //path = path_+"/nf";
    //std::map<uint64_t, uint32_t> nf_map(nf_.begin(), nf_.end());
    //izenelib::am::ssf::Util<>::Save(path, nf_map);
    path = path_+"/synonym_map";
    izenelib::am::ssf::Util<>::Save(path, synonym_pairs);
    for (size_t i = 0; i < term_set.size(); ++i)
        if (term_set[i][term_set[i].length() - 1] == '/') term_set[i].erase(term_set[i].length() - 1, 1);
    path = path_+"/synonym_dict";
    izenelib::am::ssf::Util<>::Save(path, term_set);
    {
        std::string aversion_file = path_+"/AVERSION";
        std::ofstream ofs(aversion_file.c_str());
        ofs<<AVERSION<<std::endl;
        ofs.close();
    }
    {
        std::string rversion_file = path_+"/RVERSION";
        std::ofstream ofs(rversion_file.c_str());
        ofs<<rversion<<std::endl;
        ofs.close();
    }
    SetIndexDone_(path_, true);

    return true;
}

void ProductMatcher::IndexFuzzy_()
{
    //term_index_.forward.resize(1);
    //boost::unordered_set<TermList> fuzzy_term_set;
    for(uint32_t i=1;i<products_.size();i++)
    {
        if(i%100000==0)
        {
            LOG(INFO)<<"fuzzy scanning product "<<i<<std::endl;
            //LOG(INFO)<<"max tid "<<aid_manager_->getMaxDocId()<<std::endl;
        }
        //uint32_t pid = i;
        const Product& product = products_[i];
        cid_t cid = product.cid;
        TermIndex& term_index = term_index_map_[cid];
        if(term_index.forward.empty())
        {
            term_index.forward.resize(1);
        }
        const std::vector<Attribute>& attributes = product.attributes;
        for(uint32_t a=0;a<attributes.size();a++)
        {
            const Attribute& attribute = attributes[a];
            for(uint32_t v=0;v<attribute.values.size();v++)
            {
                if(!attribute.is_optional&&NeedFuzzy_(attribute.values[v]))
                {
#ifdef B5M_DEBUG
                    std::cerr<<"need fuzzy "<<cid<<","<<attribute.values[v]<<std::endl;
#endif
                    //std::vector<term_t> terms;
                    UString uv(attribute.values[v], UString::UTF_8);
                    ATermList terms;
                    AnalyzeNoSymbol_(uv, terms);
                    TermList ids(terms.size());
                    for(uint32_t t=0;t<terms.size();t++)
                    {
                        ids[t] = terms[t].id;
                    }
                    if(term_index.set.find(ids)!=term_index.set.end()) continue;
                    term_index.set.insert(ids);
                    //if(fuzzy_term_set.find(ids)!=fuzzy_term_set.end()) continue;
                    //fuzzy_term_set.insert(ids);
                    uint32_t keyword_index = term_index.forward.size();
                    term_index.forward.push_back(terms);
                    for(uint32_t t=0;t<terms.size();t++)
                    {
                        TermIndexItem item;
                        item.keyword_index = keyword_index;
                        item.pos = t;
                        term_index.invert[ids[t]].items.push_back(item);
                    }
                }

            }
        }
    }
    for(TermIndexMap::iterator it=term_index_map_.begin();it!=term_index_map_.end();++it)
    {
        it->second.flush();
    }
    //term_index_.flush();
    //std::stable_sort(products_.begin(), products_.end(), Product::CidCompare);
    //uint32_t last_cid = 0;
    //std::pair<uint32_t, uint32_t> range(1, 1);
    //for(uint32_t i=1;i<products_.size();i++)
    //{
        //const Product& p = products_[i];
        //if(p.cid>last_cid)
        //{
            //range.second = i;
            //if(range.second>range.first)
            //{
                ////LOG(INFO)<<"index fuzzy at cid "<<last_cid<<std::endl;
                //ProcessFuzzy_(range);
            //}
            //last_cid = p.cid;
            //range.first = i;
        //}
    //}
    //range.second = products_.size();
    //if(range.second>range.first)
    //{
        //ProcessFuzzy_(range);
    //}

}

//void ProductMatcher::ProcessFuzzy_(const std::pair<uint32_t, uint32_t>& range)
//{
    //LOG(INFO)<<"process fuzzy on "<<range.first<<","<<range.second<<std::endl;
    //typedef std::map<std::vector<uint32_t>, std::vector<uint32_t> > FuzzyTrie; //key => pid_list
    //FuzzyTrie trie;
    //for(uint32_t i=range.first; i<range.second; i++)
    //{
        //const Product& p = products_[i];
        ////LOG(INFO)<<"cid "<<p.cid<<std::endl;
        //for(uint32_t a=0;a<p.attributes.size();a++)
        //{
            //const Attribute& attr = p.attributes[a];
            //if(attr.is_optional || attr.values.size()>1) continue;
            //const std::string& value = attr.values[0];
            //UString uname(attr.name, UString::UTF_8);
            //std::vector<UString> t;
            //AnalyzeChar_(uname, t);
            //std::vector<term_t> nit(t.size());
            //for(uint32_t j=0;j<t.size();j++)
            //{
                //nit[j] = GetTerm_(t[j]);
            //}
            //UString direction("forward", UString::UTF_8);
            //uint32_t did = GetTerm_(direction);
            //UString uvalue(value, UString::UTF_8);
            //t.resize(0);
            //AnalyzeChar_(uvalue, t);
            //std::vector<term_t> vit(t.size());
            //for(uint32_t j=0;j<t.size();j++)
            //{
                //vit[j] = GetTerm_(t[j]);
            //}
            //FuzzyKey fkey(p.cid, nit, did, vit);
            //std::vector<uint32_t> key = fkey.GenKey();
            //trie[key].push_back(i);
            ////FuzzyTrie::iterator it = trie.find(key);
            ////if(it==trie.end())
            ////{
                ////trie[key] = 1;
            ////}
            ////else
            ////{
                ////it->second+=1;
            ////}
        //}
    //}
    //for(FuzzyTrie::const_iterator its = trie.begin();its!=trie.end();its++)
    //{
        //FuzzyKey fkey;
        //fkey.Parse(its->first);
        //std::string text = GetText_(fkey.value_ids);
        //UString utext(text, UString::UTF_8);
        //double length = utext.length();
        //if(length<7.0) continue;
        //std::vector<uint32_t> found_prefix;
        //for(uint32_t p=0;p<fkey.value_ids.size();p++)
        //{
            ////if(fkey.value_ids.size()<3) continue;
            //std::vector<uint32_t> prefix(fkey.value_ids.begin(), fkey.value_ids.begin()+p+1);
            //std::vector<uint32_t> remain(fkey.value_ids.begin()+p+1, fkey.value_ids.end());
            //if(remain.size()<2) break;
            //std::string ptext = GetText_(prefix);
            //UString uptext(ptext, UString::UTF_8);
            //std::string rtext = GetText_(remain);
            //UString urtext(rtext, UString::UTF_8);
            //bool all_digits = true;
            //for(uint32_t i=0;i<urtext.length();i++)
            //{
                //if(!urtext.isDigitChar(i))
                //{
                    //all_digits = false;
                    //break;
                //}
            //}
            //if(all_digits) break;
            //double plength = uptext.length();
            //if(plength<=1.0) continue;
            //double r = plength/length;
            //if(r>0.5) break;
            //FuzzyTrie::const_iterator it = its;
            //uint32_t count = 0;
            //while(true)
            //{
                //FuzzyKey fkey2;
                //fkey2.Parse(it->first);
                //if(fkey2.cid!=fkey.cid) break;
                //if(fkey2.name_ids!=fkey.name_ids) break;
                //if(fkey2.did!=fkey.did) break;
                //if(!boost::algorithm::starts_with(fkey2.value_ids, prefix)) break;
                //count+=it->second.size();
                //if(it==trie.begin()) break;
                //it--;
            //}
            //it = its;
            //++it;
            //while(it!=trie.end())
            //{
                //FuzzyKey fkey2;
                //fkey2.Parse(it->first);
                //if(fkey2.cid!=fkey.cid) break;
                //if(fkey2.name_ids!=fkey.name_ids) break;
                //if(fkey2.did!=fkey.did) break;
                //if(!boost::algorithm::starts_with(fkey2.value_ids, prefix)) break;
                //count+=it->second.size();
                //++it;
            //}
            //if(count>=3)
            //{
                //found_prefix = prefix;
                ////LOG(INFO)<<"find fuzzy "<<text<<","<<ptext<<std::endl;
            //}
        //}
        //if(!found_prefix.empty())
        //{
            //LOG(INFO)<<"find fuzzy "<<text<<","<<GetText_(found_prefix)<<std::endl;
            //std::vector<uint32_t> remain(fkey.value_ids.begin()+found_prefix.size(), fkey.value_ids.end());
            //std::string new_attr_value = GetText_(remain, " ");
            //const std::vector<uint32_t>& pid_list = its->second;
            //for(uint32_t i=0;i<pid_list.size();i++)
            //{
                //Product& p = products_[pid_list[i]];
                //for(uint32_t a=0;a<p.attributes.size();a++)
                //{
                    //Attribute& attr = p.attributes[a];
                    //UString uname(attr.name, UString::UTF_8);
                    //std::vector<UString> t;
                    //AnalyzeChar_(uname, t);
                    //std::vector<term_t> nit(t.size());
                    //for(uint32_t j=0;j<t.size();j++)
                    //{
                        //nit[j] = GetTerm_(t[j]);
                    //}
                    //if(nit==fkey.name_ids)
                    //{
                        //const std::string& ovalue = attr.values[0];
                        //LOG(INFO)<<"fuzzy apply "<<p.stitle<<","<<attr.name<<" from "<<ovalue<<" to "<<new_attr_value<<std::endl;
                        //attr.values.resize(1);
                        //attr.values[0] = new_attr_value;
                        //break;
                    //}
                //}
            //}
        //}
    //}
//}

bool ProductMatcher::NeedFuzzy_(const std::string& value)
{
    UString text(value, UString::UTF_8);
    bool all_chinese = true;
    for(uint32_t i=0;i<text.length();i++)
    {
        if(!text.isChineseChar(i))
        {
            all_chinese = false;
            break;
        }
    }
    if(all_chinese)
    {
        if(text.length()<5) return false;
    }
    else if(text.length()<7) return false;
    ATermList tl;
    AnalyzeNoSymbol_(text, tl);
    if(tl.size()<3) return false;
    return true;
}

void ProductMatcher::IndexOffer_(const std::string& offer_scd)
{
    LOG(INFO)<<"index offer begin"<<std::endl;
    ScdParser parser(izenelib::util::UString::UTF_8);
    parser.load(offer_scd);
    uint32_t n=0;
    NgramFrequent nf;
    feature_vectors_.resize(category_list_.size());
    std::vector<uint32_t> fv_count(category_list_.size(), 0);
    for( ScdParser::iterator doc_iter = parser.begin();
      doc_iter!= parser.end(); ++doc_iter, ++n)
    {
        //if(n>=15000) break;
        if(n%100000==0)
        {
            LOG(INFO)<<"Find Offer Documents "<<n<<std::endl;
        }
        //Document doc;
        SCDDoc& scddoc = *(*doc_iter);
        SCDDoc::iterator p = scddoc.begin();
        Document::doc_prop_value_strtype title;
        Document::doc_prop_value_strtype category;
        for(; p!=scddoc.end(); ++p)
        {
            const std::string& property_name = p->first;
            if(property_name=="Title")
            {
                title = p->second;
            }
            else if(property_name=="Category")
            {
                category = p->second;
            }
        }
        if(category.empty()||title.empty()) continue;
        std::string scategory = propstr_to_str(category);
        CategoryIndex::const_iterator cit = category_index_.find(scategory);;
        if(cit==category_index_.end()) continue;
#ifdef B5M_DEBUG
        //std::string stitle;
        //title.convertString(stitle, UString::UTF_8);
        //std::cerr<<"index offer title "<<stitle<<std::endl;
#endif
        uint32_t cid = cit->second;
        OfferCategoryApp app;
        app.cid = cid;
        app.count = 1;
        std::vector<Term> term_list;
        Analyze_(propstr_to_ustr(title), term_list);
        KeywordVector keyword_vector;
        GetKeywords(term_list, keyword_vector, false);
        for(uint32_t i=0;i<keyword_vector.size();i++)
        {
            const TermList& tl = keyword_vector[i].term_list;
            trie_[tl].offer_category_apps.push_back(app);
        }
        //process feature vector
        cid_t cid1 = GetLevelCid_(scategory, 1);
        cid_t cid2 = GetLevelCid_(scategory, 2);
        FeatureVector feature_vector;
        GenFeatureVector_(keyword_vector, feature_vector);
        if(cid1!=0)
        {
            FeatureVectorAdd_(feature_vectors_[cid1], feature_vector);
            fv_count[cid1]++;
        }
        if(cid2!=0)
        {
            FeatureVectorAdd_(feature_vectors_[cid2], feature_vector);
            fv_count[cid2]++;
        }
        

        if(use_ngram_)
        {
            for(uint32_t i=0;i<keyword_vector.size();i++)
            {
                const KeywordTag& ki = keyword_vector[i];
                for(uint32_t j=i+1;j<keyword_vector.size();j++)
                {
                    const KeywordTag& kj = keyword_vector[j];
                    std::vector<uint32_t> ids(2);
                    ids[0] = ki.id;
                    ids[1] = kj.id;
                    if(ids[1]<ids[0]) std::swap(ids[0], ids[1]);
                    nf[ids].push_back(std::make_pair(cid, 1));
                }
            }
        }

    }
    for(uint32_t i=0;i<feature_vectors_.size();i++)
    {
        FeatureVector& feature_vector = feature_vectors_[i];
        for(uint32_t j=0;j<feature_vector.size();j++)
        {
            feature_vector[j].second/=fv_count[i];
        }
        FeatureVectorNorm_(feature_vector);
    }
    for(TrieType::iterator it = trie_.begin();it!=trie_.end();it++)
    {
        KeywordTag& tag = it->second;
        std::sort(tag.offer_category_apps.begin(), tag.offer_category_apps.end());
        std::vector<OfferCategoryApp> new_apps;
        new_apps.reserve(tag.offer_category_apps.size());
        OfferCategoryApp last;
        last.cid = 0;
        last.count = 0;
        for(uint32_t i=0;i<tag.offer_category_apps.size();i++)
        {
            const OfferCategoryApp& app = tag.offer_category_apps[i];
            if(app.cid!=last.cid)
            {
                if(last.count>0)
                {
                    new_apps.push_back(last);
                }
                last.cid = app.cid;
                last.count = 0;
            }
            last.count+=app.count;
        }
        if(last.count>0)
        {
            new_apps.push_back(last);
        }
        tag.offer_category_apps.swap(new_apps);
    }

    if(use_ngram_)
    {
        uint32_t new_kid = all_keywords_.size();
        for(NgramFrequent::iterator it = nf.begin();it!=nf.end();it++)
        {
            const std::vector<uint32_t>& ids = it->first;
            FrequentValue& value = it->second;
            std::sort(value.begin(), value.end());
            FrequentValue new_value;
            new_value.reserve(value.size());
            for(uint32_t i=0;i<value.size();i++)
            {
                const std::pair<uint32_t, uint32_t>& v = value[i];
                if(new_value.empty())
                {
                    new_value.push_back(v);
                }
                else
                {
                    if(new_value.back().first==v.first)
                    {
                        new_value.back().second+=v.second;
                    }
                    else
                    {
                        new_value.push_back(v);
                    }
                }
            }
            uint32_t freq = 0;
            for(uint32_t i=0;i<new_value.size();i++)
            {
                const std::pair<uint32_t, uint32_t>& v = new_value[i];
                freq += v.second;
            }
            double entropy = 0.0;
            for(uint32_t i=0;i<new_value.size();i++)
            {
                const std::pair<uint32_t, uint32_t>& v = new_value[i];
                double p = (double)v.second/freq;
                entropy += p*std::log(p);
            }
            entropy*=-1.0;

            if(freq>=20 && entropy<1.0)
            {
#ifdef B5M_DEBUG
                std::cerr<<"NGRAM";
                for(uint32_t i=0;i<ids.size();i++)
                {
                    uint32_t id = ids[i];
                    const KeywordTag& k = all_keywords_[id];
                    std::cerr<<","<<GetText_(k.term_list);
                }
                std::cerr<<" - "<<freq<<","<<entropy<<std::endl;
#endif

                double rev_entropy = 10.0;
                if(entropy>0.0)
                {
                    rev_entropy = 1.0/entropy;
                }
                if(rev_entropy>10.0) rev_entropy = 10.0;
                double weight = std::log((double)freq)*rev_entropy;
                KeywordTag ngram_tag;
                ngram_tag.id = new_kid++;
                ngram_tag.kweight = weight;
                for(uint32_t i=0;i<ids.size();i++)
                {
                    uint32_t id = ids[i];
                    const KeywordTag& k = all_keywords_[id];
                    ngram_tag.term_list.push_back(0);
                    ngram_tag.term_list.insert(ngram_tag.term_list.end(), k.term_list.begin(), k.term_list.end());

                }
                for(uint32_t i=0;i<new_value.size();i++)
                {
                    const std::pair<uint32_t, uint32_t>& v = new_value[i];
                    OfferCategoryApp app;
                    app.cid = v.first;
                    app.count = v.second;
                    ngram_tag.offer_category_apps.push_back(app);
                }
                trie_[ngram_tag.term_list] = ngram_tag;

            }
        }
        LOG(INFO)<<"find ngram keyword "<<new_kid-all_keywords_.size()<<std::endl;
    }
    LOG(INFO)<<"index offer end"<<std::endl;
}

bool ProductMatcher::OutputCategoryMap(const std::string& scd_path, const std::string& output_file)
{
    izenelib::util::ClockTimer clocker;
    std::vector<std::string> scd_list;
    B5MHelper::GetIUScdList(scd_path, scd_list);
    if(scd_list.empty()) return false;
    std::ofstream ofs(output_file.c_str());
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
            if(n%10000==0)
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
            std::string scategory;
            doc.getString("Category", scategory);
            if(scategory.empty()) continue;
            std::string socategory;
            doc.getString("OriginalCategory", socategory);
            boost::algorithm::replace_all(socategory, "\t", "");
            boost::algorithm::replace_all(socategory, " ", "");
            if(socategory.empty()) continue;
            std::string ssource;
            doc.getString("Source", ssource);
            if(ssource.empty()) continue;
            doc.eraseProperty("Category");
            std::string key = ssource+","+socategory;

            Product result_product;
            Process(doc, result_product, true);
            doc.property("Category") = str_to_propstr(scategory);
            std::string srcategory = result_product.scategory;
            if(!srcategory.empty())
            {
                doc.property("PCategory") = str_to_propstr(srcategory);
            }
            UString value_text;
            JsonDocument::ToJsonText(doc, value_text);
            std::string str;
            value_text.convertString(str, UString::UTF_8);
            ofs<<key<<"\t"<<str<<std::endl;
        }
    }
    ofs.close();
    LOG(INFO)<<"clocker used "<<clocker.elapsed()<<std::endl;
    return true;
}

bool ProductMatcher::DoMatch(const std::string& scd_path, const std::string& output_file)
{
    izenelib::util::ClockTimer clocker;
    std::vector<std::string> scd_list;
    B5MHelper::GetIUScdList(scd_path, scd_list);
    if(scd_list.empty()) return false;
    std::string match_file = output_file;
    if(match_file.empty())
    {
        match_file = path_+"/match";
    }
    std::ofstream ofs(match_file.c_str());
    std::size_t doc_count=0;
    std::size_t spu_matched_count=0;
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
            if(n%10000==0)
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
            Product result_product;
            Process(doc, result_product, true);
            doc_count++;
            std::string spid = result_product.spid;
            std::string sptitle = result_product.stitle;
            std::string scategory = result_product.scategory;
            std::string soid;
            std::string stitle;
            doc.getString("DOCID", soid);
            doc.getString("Title", stitle);
            if(spid.length()>0)
            {
                spu_matched_count++;
                //ofs<<soid<<","<<spid<<","<<stitle<<","<<sptitle<<","<<scategory<<std::endl;
            }
            //else
            //{
                //ofs<<soid<<","<<stitle<<","<<scategory<<std::endl;
            //}
            ofs<<soid<<","<<spid<<","<<stitle<<","<<sptitle<<","<<scategory<<std::endl;
        }
    }
    ofs.close();
    LOG(INFO)<<"clocker used "<<clocker.elapsed()<<std::endl;
    LOG(INFO)<<"stat: doc_count:"<<doc_count<<", spu_matched_count:"<<spu_matched_count<<std::endl;
    return true;
}

bool ProductMatcher::FuzzyDiff(const std::string& scd_path, const std::string& output_file)
{
    izenelib::util::ClockTimer clocker;
    std::vector<std::string> scd_list;
    B5MHelper::GetIUScdList(scd_path, scd_list);
    if(scd_list.empty()) return false;
    std::string diff_file = output_file;
    if(diff_file.empty())
    {
        diff_file = path_+"/fuzzy_diff";
    }
    std::ofstream ofs(diff_file.c_str());
    std::size_t positive = 0;
    std::size_t negative = 0;
    std::size_t doc_count=0;
    std::size_t both_spu_matched = 0;
    std::size_t both_book_matched = 0;
    double no_f_time = 0.0;
    double f_time = 0.0;
    izenelib::util::ClockTimer inner_clocker;

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
            if(n%10000==0)
            {
                LOG(INFO)<<"Find Offer Documents "<<n<<std::endl;
                LOG(INFO)<<"clocker used "<<clocker.elapsed()<<","<<no_f_time<<","<<f_time<<std::endl;
            }
            SCDDoc& scddoc = *(*doc_iter);
            SCDDoc::iterator p = scddoc.begin();
            Document doc;
            for(; p!=scddoc.end(); ++p)
            {
                const std::string& property_name = p->first;
                doc.property(property_name) = p->second;
            }
            Product result_product;
            inner_clocker.restart();
            Process(doc, result_product, false);
            no_f_time += inner_clocker.elapsed();
            doc_count++;
            std::string soid;
            std::string stitle;
            doc.getString("DOCID", soid);
            doc.getString("Title", stitle);
            bool matched = false;
            bool fmatched = false;
            std::string spid = result_product.spid;
            std::string sptitle = result_product.stitle;
            if(!spid.empty())
            {
                matched = true;
            }
            inner_clocker.restart();
            Process(doc, result_product, true);
            f_time += inner_clocker.elapsed();
            std::string sfpid = result_product.spid;
            std::string sfptitle = result_product.stitle;
            if(!sfpid.empty())
            {
                fmatched = true;
            }
            if(matched!=fmatched)
            {
                if(matched)
                {
                    negative++;
                    ofs<<"N,"<<stitle<<","<<sptitle<<std::endl;
                }
                else
                {
                    positive++;
                    ofs<<"P,"<<stitle<<","<<sfptitle<<std::endl;
                }
            }
            else if(matched&&fmatched)
            {
                if(sptitle.empty())
                {
                    both_book_matched++;
                }
                else
                {
                    both_spu_matched++;
                }
            }
        }
    }
    ofs.close();
    LOG(INFO)<<"clocker used "<<clocker.elapsed()<<","<<no_f_time<<","<<f_time<<std::endl;
    LOG(INFO)<<"stat: doc_count:"<<doc_count<<", positive:"<<positive<<", negative:"<<negative<<", both spu:"<<both_spu_matched<<", both book:"<<both_book_matched<<std::endl;
    return true;
}
void ProductMatcher::Test(const std::string& scd_path)
{
    //{
        //std::ifstream ifs(scd_path.c_str());
        //ScdWriter writer(".", INSERT_SCD);
        //std::string line;
        //while(getline(ifs, line))
        //{
            //boost::algorithm::trim(line);
            //Document doc;
            //doc.property("DOCID") = UString("00000000000000000000000000000000", UString::UTF_8);
            //doc.property("Title") = UString(line, UString::UTF_8);
            //SetUsePriceSim(false);
            //Product result_product;
            //Process(doc, result_product);
            //doc.property("Category") = UString(result_product.scategory, UString::UTF_8);
            //writer.Append(doc);
            //std::cerr<<result_product.scategory<<std::endl;

        //}
        //writer.Close();
        //return;
    //}
    std::vector<std::pair<std::string, std::string> > process_list;
    bfs::path d(scd_path);
    if(!bfs::is_directory(d)) return;
    std::vector<std::string> scd_list;
    B5MHelper::GetIUScdList(scd_path, scd_list);
    if(scd_list.size()==1)
    {
        process_list.push_back(std::make_pair(d.filename().string(), scd_list.front()));
    }
    bfs::path p(scd_path);
    bfs::directory_iterator end;
    for(bfs::directory_iterator it(p);it!=end;it++)
    {
        bfs::path d = it->path();
        if(bfs::is_directory(d))
        {
            std::string test_dir = d.string();
            std::string test_name = d.filename().string();
            scd_list.clear();
            B5MHelper::GetIUScdList(test_dir, scd_list);
            if(scd_list.size()==1)
            {
                process_list.push_back(std::make_pair(test_name, scd_list.front()));
            }
        }
    }
    if(process_list.empty()) return;
    boost::posix_time::ptime now(boost::posix_time::microsec_clock::local_time());
    std::string ios_str = boost::posix_time::to_iso_string(now);
    std::string ts;
    ts += ios_str.substr(0,8);
    ts += ios_str.substr(9,6);
    std::string output_dir = "./matcher_test_result";
    boost::filesystem::create_directories(output_dir);
    std::string output_file = output_dir+"/"+ts+".json";
    std::ofstream ofs(output_file.c_str());
    for(uint32_t i=0;i<process_list.size();i++)
    {
        izenelib::util::ClockTimer clocker;
        uint32_t allc = 0;
        uint32_t correctc = 0;
        uint32_t allp = 0;
        uint32_t correctp = 0;
        //std::ofstream ofs("./brand.txt");
        //for(uint32_t i=0;i<products_.size();i++)
        //{
            //Product& p = products_[i];
            //std::vector<Attribute>& attributes = p.attributes;
            //for(uint32_t a=0;a<attributes.size();a++)
            //{
                //Attribute& attribute = attributes[a];
                //if(attribute.name=="品牌")
                //{
                    //std::string value;
                    //for(uint32_t v=0;v<attribute.values.size();v++)
                    //{
                        //if(!value.empty()) value+="/";
                        //value+=attribute.values[v];
                    //}
                    //ofs<<value<<std::endl;
                //}
            //}
        //}
        //ofs.close();
        std::string test_name = process_list[i].first;
        std::string scd_file = process_list[i].second;
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
            SCDDoc& scddoc = *(*doc_iter);
            SCDDoc::iterator p = scddoc.begin();
            Document doc;
            for(; p!=scddoc.end(); ++p)
            {
                const std::string& property_name = p->first;
                doc.property(property_name) = p->second;
            }
            if(doc.hasProperty("Price"))
            {
                SetUsePriceSim(true);
            }
            else
            {
                SetUsePriceSim(false);
            }
            bool has_category = false;
            bool has_pid = false;
            if(doc.hasProperty("Category"))
            {
                has_category = true;
            }
            if(doc.hasProperty("uuid"))
            {
                has_pid = true;
            }
            std::string sdocid;
            doc.getString("DOCID", sdocid);
            std::string ecategory;
            doc.getString("Category", ecategory);
            std::string stitle;
            doc.getString("Title", stitle);
            std::string epid;
            doc.getString("uuid", epid);
            Product result_product;
            doc.eraseProperty("Category");
            if(!has_category&&!has_pid)
            {
                uint32_t limit = 3;
                std::vector<UString> frontends;
                UString title(stitle, UString::UTF_8);
                GetFrontendCategory(title, limit, frontends);
                std::cerr<<"frontend results of "<<stitle<<": ";
                for(uint32_t i=0;i<frontends.size();i++)
                {
                    std::string str;
                    frontends[i].convertString(str, UString::UTF_8);
                    std::cerr<<str<<",";
                }
                std::cerr<<std::endl;
            }
            else
            {
                Process(doc, result_product, true);
                //LOG(INFO)<<"categorized "<<stitle<<","<<result_product.scategory<<std::endl;
                if(has_category)
                {
                    allc++;
                    if(boost::algorithm::starts_with(result_product.scategory,ecategory))
                    {
                        correctc++;
                    }
                    else
                    {
                        LOG(ERROR)<<"category error : "<<sdocid<<","<<stitle<<","<<ecategory<<","<<result_product.scategory<<std::endl;
                    }
                }
                if(has_pid)
                {
                    allp++;
                    if(result_product.spid==epid)
                    {
                        correctp++;
                    }
                    else
                    {
                        LOG(ERROR)<<"spu error : "<<sdocid<<","<<stitle<<","<<epid<<","<<result_product.spid<<std::endl;
                    }
                }
            }
        }
        double ratioc = (double)correctc/allc;
        double ratiop = (double)correctp/allp;
        LOG(INFO)<<"stat : "<<allc<<","<<correctc<<","<<allp<<","<<correctp<<std::endl;
        double time_used = clocker.elapsed();
        LOG(INFO)<<"clocker used "<<clocker.elapsed()<<std::endl;
        Json::Value json_value;
        json_value["name"] = test_name;
        json_value["allc"] = Json::Value::UInt(allc);
        json_value["correctc"] = Json::Value::UInt(correctc);
        json_value["allp"] = Json::Value::UInt(allp);
        json_value["correctp"] = Json::Value::UInt(correctp);
        json_value["ratioc"] = ratioc;
        json_value["ratiop"] = ratiop;
        json_value["time"] = time_used;
        Json::FastWriter writer;
        std::string str_value = writer.write(json_value);
        boost::algorithm::trim(str_value);
        ofs<<str_value<<std::endl;
    }
    ofs.close();
}
bool ProductMatcher::GetIsbnAttribute(const Document& doc, std::string& isbn_value)
{
    const static std::string isbn_name = "isbn";
    Document::doc_prop_value_strtype attrib_ustr;
    doc.getProperty("Attribute", attrib_ustr);
    std::vector<Attribute> attributes;
    ParseAttributes(propstr_to_ustr(attrib_ustr), attributes);
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
    if(!isbn_value.empty()) return true;
    return false;

}

bool ProductMatcher::ProcessBook(const Document& doc, Product& result_product)
{
    std::string scategory;
    doc.getString("Category", scategory);
    if(boost::algorithm::starts_with(scategory, book_category_))
    {
        std::string isbn_value;
        GetIsbnAttribute(doc, isbn_value);
        if(!isbn_value.empty())
        {
            result_product.spid = B5MHelper::GetPidByIsbn(isbn_value);
        }
        return true;
    }
    else if(scategory.empty())
    {
        std::string isbn_value;
        GetIsbnAttribute(doc, isbn_value);
        if(!isbn_value.empty())
        {
            result_product.spid = B5MHelper::GetPidByIsbn(isbn_value);
            result_product.scategory = book_category_;
            return true;
        }
    }
    return false;
}

bool ProductMatcher::ProcessBookStatic(const Document& doc, Product& result_product)
{
    std::string isbn_value;
    GetIsbnAttribute(doc, isbn_value);
    if(!isbn_value.empty())
    {
        result_product.spid = B5MHelper::GetPidByIsbn(isbn_value);
        return true;
    }
    return false;
}

bool ProductMatcher::Process(const Document& doc, Product& result_product, bool use_fuzzy)
{
    static const uint32_t limit = 1;
    std::vector<Product> products;
    if(Process(doc, limit, products, use_fuzzy) && !products.empty())
    {
        result_product = products.front();
        return true;
    }
    return false;
}

bool ProductMatcher::Process(const Document& doc, uint32_t limit, std::vector<Product>& result_products, bool use_fuzzy)
{
    if(!IsOpen()) return false;
    if(limit==0) return false;
    Product pbook;
    if(ProcessBook(doc, pbook))
    {
        result_products.resize(1, pbook);
        return true;
    }
    Document::doc_prop_value_strtype ptitle;
    Document::doc_prop_value_strtype pcategory;
    doc.getProperty("Category", pcategory);
    doc.getProperty("Title", ptitle);
    UString title = propstr_to_ustr(ptitle);
    UString category = propstr_to_ustr(pcategory);

    if(title.length()==0)
    {
        return false;
    }
    cid_t cid = GetCid_(category);
    //keyword_vector_.resize(0);
    //std::cout<<"[TITLE]"<<stitle<<std::endl;
    std::vector<Term> term_list;
    Analyze_(title, term_list);
    KeywordVector keyword_vector;
    //GetKeywords_(term_list, keyword_vector, use_ngram_, true);
    GetKeywords(term_list, keyword_vector, use_fuzzy, cid);
    Compute2_(doc, term_list, keyword_vector, limit, result_products);
    for(uint32_t i=0;i<result_products.size();i++)
    {
        //gen frontend category
        Product& p = result_products[i];
        UString frontend;
        UString backend(p.scategory, UString::UTF_8);
        GetFrontendCategory(backend, frontend);
        backend.convertString(p.scategory, UString::UTF_8);
        frontend.convertString(p.fcategory, UString::UTF_8);
    }
    return true;
}

void ProductMatcher::GetFrontendCategory(const UString& text, uint32_t limit, std::vector<UString>& results)
{
    if(!IsOpen()) return;
    if(limit==0) return;
    if(text.length()==0) return;
    Document doc;
    doc.property("Title") = ustr_to_propstr(text);
    const UString& title = text;

    std::vector<Term> term_list;
    Analyze_(title, term_list);
    KeywordVector keyword_vector;
    GetKeywords(term_list, keyword_vector, false);
    //std::cerr<<"keywords count "<<keyword_vector.size()<<std::endl;
    uint32_t flimit = limit*2;
    std::vector<Product> result_products;
    Compute2_(doc, term_list, keyword_vector, flimit, result_products);
    for(uint32_t i=0;i<result_products.size();i++)
    {
        Product& p = result_products[i];
        UString frontend;
        UString backend(p.scategory, UString::UTF_8);
        GetFrontendCategory(backend, frontend);
        //std::cerr<<"find backend "<<p.scategory<<std::endl;
        if(frontend.length()==0)
        {
            p.score = 0.0;
            continue;
        }
        frontend.convertString(p.fcategory, UString::UTF_8);
        //std::cerr<<"find frontend "<<p.fcategory<<std::endl;
    }
    std::stable_sort(result_products.begin(), result_products.end(), Product::FCategoryCompare);

    for(uint32_t i=0;i<result_products.size();i++)
    {
        Product& pi = result_products[i];
        if(pi.score==0.0) continue;
        for(uint32_t j=i+1;j<result_products.size();j++)
        {
            Product& pj = result_products[j];
            if(pj.score==0.0) continue;
            if(pi.fcategory==pj.fcategory)
            {
                pj.score = 0.0;
            }
            else if(boost::algorithm::starts_with(pj.fcategory, pi.fcategory))
            {
                if(pi.score>=pj.score*0.85)
                {
                    pj.score = 0.0;
                }
                else
                {
                    pi.score = 0.0;
                }
            }
        }
    }
    std::sort(result_products.begin(), result_products.end(), Product::ScoreCompare);
    for(uint32_t i=0;i<result_products.size();i++)
    {
        Product& p = result_products[i];
        if(p.score>0.0)
        {
            UString frontend(p.fcategory, UString::UTF_8);
            results.push_back(frontend);
            if(results.size()>=limit) break;
        }
    }
    //typedef std::map<std::string, double> FrontApp;
    //boost::unordered_set<std::string> front_app;
    //FrontApp front_app;
    //for(uint32_t i=0;i<result_products.size();i++)
    //{
        //Product& p = result_products[i];
        //UString frontend;
        //UString backend(p.scategory, UString::UTF_8);
        //GetFrontendCategory(backend, frontend);
        //if(frontend.length()==0) continue;
        ////backend.convertString(p.scategory, UString::UTF_8);
        //frontend.convertString(p.fcategory, UString::UTF_8);
        //CategoryDepthCut(p.fcategory, category_max_depth_);
        //FrontApp::iterator it = front_app.lower_bound(p.fcategory);
        //if(it==front_app.end())
        //{
            //front_app.insert(std::make_pair(p.fcategory, p.score));
        //}
        //else
        //{
            //if(it->first==p.fcategory) //duplicate
            //{
                ////ignore
            //}
            //else
            //{
                ////is child
            //}
        //}
        //if(front_app.find(p.fcategory)!=front_app.end()) continue;
        //results.push_back(frontend);
        //front_app.insert(p.fcategory);
        //if(results.size()==limit) break;
    //}
}
bool ProductMatcher::GetKeyword(const UString& text, KeywordTag& tag)
{
    std::vector<Term> term_list;
    Analyze_(text, term_list);
    std::vector<term_t> ids(term_list.size());
    for(uint32_t i=0;i<ids.size();i++)
    {
        ids[i] = term_list[i].id;
    }
    TrieType::const_iterator it = trie_.find(ids);
    if(it!=trie_.end())
    {
        tag = it->second;
        return true;
    }
    return false;
}

void ProductMatcher::GetKeywords(const ATermList& term_list, KeywordVector& keyword_vector, bool bfuzzy, cid_t cid)
{
    //std::string stitle;
    //text.convertString(stitle, UString::UTF_8);
    uint32_t begin = 0;
    uint8_t bracket_depth = 0;
    uint32_t last_complete_keyword_fol = 0;
    keyword_vector.reserve(10);
    //typedef boost::unordered_map<TermList, uint32_t> KeywordIndex;
    //typedef KeywordVector::value_type ItemType;
    //typedef std::pair<double, std::string> ItemScore;
    //KeywordIndex keyword_index;
    //KeywordAppMap app_map;
    while(begin<term_list.size())
    {
        //if(bracket_depth>0)
        //{
            //begin++;
            //continue;
        //}
        //bool found_keyword = false;
        //typedef std::vector<ItemType> Buffer;
        //Buffer buffer;
        //buffer.reserve(5);
        //std::pair<TrieType::key_type, TrieType::mapped_type> value;
        KeywordTag found_keyword;
        uint32_t keyword_fol_position = 0;
        std::vector<term_t> candidate_keyword;
        std::vector<Term> candidate_terms;
        candidate_keyword.reserve(10);
        candidate_terms.reserve(10);
        uint32_t next_pos = begin;
        Term last_term;
        //std::cerr<<"[BEGIN]"<<begin<<std::endl;
        while(true)
        {
            if(next_pos>=term_list.size()) break;
            const Term& term = term_list[next_pos++];
            //std::string sterm = GetText_(term);
            //std::cerr<<"[A]"<<sterm<<std::endl;
            if(IsSymbol_(term))
            {
                if(IsConnectSymbol_(term.id))
                {
                    if(candidate_keyword.empty())
                    {
                        //std::cerr<<"[B0]"<<sterm<<","<<term<<std::endl;
                        break;
                    }
                    else
                    {
                        last_term = term;
                        //std::cerr<<"[B]"<<sterm<<","<<term<<std::endl;
                        continue;
                    }
                }
                else
                {
                    if(term.id==left_bracket_term_||term.id==right_bracket_term_)
                    {
                        if(next_pos-begin==1)
                        {
                            if(term.id==left_bracket_term_)
                            {
                                bracket_depth++;
                            }
                            else if(term.id==right_bracket_term_)
                            {
                                if(bracket_depth>0)
                                {
                                    bracket_depth--;
                                }
                            }
                        }
                    }
                    //std::cerr<<"[C]"<<sterm<<","<<term<<std::endl;
                    break;
                }
            }
            else
            {
                if(last_term.id==blank_term_&&!candidate_keyword.empty())
                {
                    const Term& last_candidate_term = candidate_terms.back();
                    //UString last_candidate = UString(GetText_(last_candidate_term), UString::UTF_8);
                    //UString this_text = UString(GetText_(term), UString::UTF_8);
                    if(IsBlankSplit_(last_candidate_term.text, term.text))
                    {
                        break;
                    }
                    //std::cerr<<"[D]"<<sterm<<","<<term<<std::endl;
                }
            }
            //std::cerr<<"[E]"<<sterm<<","<<term<<std::endl;
            candidate_keyword.push_back(term.id);
            candidate_terms.push_back(term);
            //std::string skeyword = GetText_(candidate_keyword);
            //std::cerr<<"[SK]"<<skeyword<<std::endl;
            TrieType::const_iterator it = trie_.lower_bound(candidate_keyword);
            if(it!=trie_.end())
            {
                if(candidate_keyword==it->first)
                {
                    //ItemType item;
                    //item.first = it->first;
                    //item.second = it->second;
                    KeywordTag tag = it->second;
                    tag.ngram = 1;
                    tag.kweight = 1.0;
                    Position pos(begin, next_pos);
                    tag.positions.push_back(pos);
                    if(bracket_depth>0) tag.kweight=0.1;
                    bool need_append = true;
                    for(uint32_t i=0;i<keyword_vector.size();i++)
                    {
                        KeywordTag& ek = keyword_vector[i];
                        if(tag.id==ek.id) //duplidate
                        {
                            if(tag.kweight>ek.kweight)
                            {
                                ek.kweight = tag.kweight;
                            }
                            ek.PositionMerge(pos);
                            need_append = false;
                            break;
                        }
                    }
                    if(need_append)
                    {
                        Term pre_term;
                        Term fol_term;
                        if(begin>0)
                        {
                            pre_term = term_list[begin-1];
                        }
                        if(next_pos<term_list.size())
                        {
                            fol_term = term_list[next_pos];
                        }
                        if(pre_term.TextString()=="." || fol_term.TextString()==".")
                        {
                            //std::cerr<<"[F]"<<pre_str<<","<<fol_str<<std::endl;
                            need_append = false;
                        }
                    }
                    if(need_append&&found_keyword.term_list.size()>0)
                    {
                        if((double)found_keyword.attribute_apps.size()/tag.attribute_apps.size()>=1.0)
                        {
                            found_keyword.kweight = 0.0;
                            keyword_vector.push_back(found_keyword);
                        }
                    }
                    //for(uint32_t i=0;i<keyword_vector.size();i++)
                    //{
                        //int select = SelectKeyword_(tag, keyword_vector[i]);
                        //if(select==1)
                        //{
                            //keyword_vector[i] = tag;
                            //need_append = false;
                            //break;
                        //}
                        //else if(select==2)
                        //{
                            //need_append = false;
                            //break;
                        //}
                        //else if(select==3)
                        //{
                            ////keyword_vector.push_back(tag);
                        //}
                    //}
                    if(need_append)
                    {
                        //keyword_vector.push_back(tag);
                        //std::cerr<<"[K]"<<sterm<<","<<term<<std::endl;
                        found_keyword = tag;
                        keyword_fol_position = next_pos;
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
            last_term = term;
        }
        if(found_keyword.term_list.size()>0)
        {
            if(begin>=last_complete_keyword_fol)
            {
                last_complete_keyword_fol = keyword_fol_position;
            }
            else
            {
                found_keyword.kweight = 0.0;//not complete keyword
            }
            keyword_vector.push_back(found_keyword);
            //begin = keyword_fol_position;
            ++begin;
        }
        else
        {
            //begin = next_pos;
            ++begin;
        }
        //begin = next_pos;
    }



    if(use_ngram_)
    {
        uint32_t keyword_count = keyword_vector.size();
        std::vector<KeywordTag> ngram_keywords;
        for(uint32_t i=0;i<keyword_count;i++)
        {
            const KeywordTag& ki = keyword_vector[i];
            for(uint32_t j=i+1;j<keyword_count;j++)
            {
                const KeywordTag& kj = keyword_vector[j];
                std::vector<uint32_t> ngram_term_list;
                ngram_term_list.reserve(ki.term_list.size()+kj.term_list.size()+2);
                ngram_term_list.push_back(0);
                const KeywordTag* ks = &ki;
                const KeywordTag* kl = &kj;
                if(ki.id>kj.id)
                {
                    ks = &kj;
                    kl = &ki;
                }
                ngram_term_list.insert(ngram_term_list.end(), ks->term_list.begin(), ks->term_list.end());
                ngram_term_list.push_back(0);
                ngram_term_list.insert(ngram_term_list.end(), kl->term_list.begin(), kl->term_list.end());
                TrieType::const_iterator it = trie_.find(ngram_term_list);
                if(it!=trie_.end())
                {
                    KeywordTag ngramk = it->second;
                    ngramk.ngram = 2;
                    ngram_keywords.push_back(ngramk);
                }

            }
        }
        std::sort(ngram_keywords.begin(), ngram_keywords.end(), KeywordTag::WeightCompare);
        std::size_t ngram_count = std::min(2ul, ngram_keywords.size());
        for(std::size_t i=0;i<ngram_count;i++)
        {
            KeywordTag& k = ngram_keywords[i];
            k.kweight = 1.0;
            keyword_vector.push_back(k);
        }
    }
    if(bfuzzy&&cid>0)
    {
        GetFuzzyKeywords_(term_list, keyword_vector, cid);
    }

}
void ProductMatcher::ExtractKeywordsFromPage(const UString& text, std::list<std::pair<UString, std::pair<uint32_t, uint32_t> > >& res_ca, std::list<std::pair<UString, std::pair<uint32_t, uint32_t> > >& res_brand, std::list<std::pair<UString, std::pair<uint32_t, uint32_t> > >& res_model)
{
    if(!IsOpen()) return;
    if(text.length() == 0) return;
    LOG(INFO)<<"length: "<<text.length()<<endl;
    uint32_t Len = 250;
    uint32_t category_size = 2;
    uint32_t sample_capacity = 25;
    double similarity_threshold = 0.05;
    ATermList term_list;
    Analyze_(text, term_list);
/*
    for(uint32_t i=0;i<term_list.size();i++)
    {
        std::string str;
        term_list[i].text.convertString(str, izenelib::util::UString::UTF_8);
        LOG(INFO)<<"i: " << i << "term: "<<str<<" pos: "<<term_list[i].position<<endl; 
    }
*/    
    KeywordVector keyword_vector;
    GetKeywords(term_list, keyword_vector, false);
    
    FeatureVector feature_vector;
    GenFeatureVector_(keyword_vector, feature_vector);
    
    LOG(INFO) <<"first_level_category_ size: " <<first_level_category_.size()<<endl;
    std::vector<std::pair<cid_t, double> > cos_value;
    boost::unordered_map<cid_t, uint32_t>::iterator it;
    for(it=first_level_category_.begin();it!=first_level_category_.end();it++)
    {
        double tcos = Cosine_(feature_vector, feature_vectors_[it->first]);
        std::vector<std::pair<cid_t, double> >::iterator iter;
        for(iter = cos_value.begin();iter!=cos_value.end();iter++)
        {
            if(iter->second < tcos)
                break;
        }
        cos_value.insert(iter, make_pair(it->first, tcos));
    }
    category_size = cos_value.size()/6;
    for(uint32_t i=0;i<cos_value.size();i++)
    {
        cout<<"i:  "<<i<<"   cos: "<<cos_value[i].second<<" cid: "<<cos_value[i].first<<" category: "
                 <<category_list_[cos_value[i].first].name<<endl;
    }

    KeywordVector temp_k;
    LOG(INFO)<<"keyword size: "<<keyword_vector.size()<<endl;
    for(uint32_t i=0;i<keyword_vector.size();i++)
    {
        KeywordTag& ki = keyword_vector[i];
        uint32_t product_count = ki.attribute_apps.size();

        std::string str;
        ki.text.convertString(str, izenelib::util::UString::UTF_8);
/*
        LOG(INFO)<<"keyword: "<<str<<"  "<<ki.positions[0].begin<<"  "<<ki.positions[0].end<<endl;
*/
        LOG(INFO)<<"keyword: "<<str<<" product count: "<<product_count<<endl;

        if(str.at(str.size()-1) == '#')
        {
            std::string st;
            term_list[ki.positions[0].end-1].text.convertString(st, izenelib::util::UString::UTF_8);
            if(st.at(st.size()-1) != '#' && st.at(0) >= '0' && st.at(0) <= '9')
            {
                str = str.substr(0, str.size()-1);
                ki.text.assign(str, izenelib::util::UString::UTF_8);
            }        
        }
        uint32_t j = 0;
        uint32_t step = product_count/sample_capacity;
        bool right_category = false;
        if(step < 1) step = 1;
        while(j < product_count)
        {
            
//            LOG(INFO)<<str<<" 属性名：  "<<ki.attribute_apps[j].attribute_name<<endl;
            bool b = false;
            if(text.length() > Len)
            {
	        cid_t cid = products_[ki.attribute_apps[j].spu_id].cid;
                cid = GetLevelCid_(category_list_[cid].name, 1);    
                for(uint32_t k=0;k< category_size;k++)
                {
                    if(cos_value[k].second < similarity_threshold)break;
		    if(cos_value[k].first == cid)
                    {
                        b = true;
                        break;
                    }
                }
            }
            else b = true;
            if(b)
            {
                right_category = true;
                if(ki.attribute_apps[j].attribute_name == "品牌" || ki.attribute_apps[j].attribute_name == "型号")
                {
                    ki.attribute_apps[0] = ki.attribute_apps[j];
//                    LOG(INFO)<<"匹配到了品牌或者型号"<<endl;
                    break;
                }
            }
            j+=step;
        }
        if(ki.attribute_apps.empty() || !right_category)continue;

        std::vector<Position>::iterator it;
        for(it = ki.positions.begin();it!=ki.positions.end();it++)
        {
            KeywordTag k = ki;
            k.positions.clear();
            k.positions.push_back(*it);
            KeywordVector::iterator iter;
            for(iter = temp_k.begin();iter!=temp_k.end();iter++)
            {
                KeywordVector::iterator itt = iter;
                itt++;
		if((itt == temp_k.end() || itt->positions[0].begin >= k.positions[0].begin) && k.positions[0].begin >= iter->positions[0].end)
                {
                    while(itt != temp_k.end())
                    {
                        if(itt->positions[0].begin < k.positions[0].end)
                        {
                            temp_k.erase(itt);
                            
                        }else break;
                    }
                    iter++;
                    temp_k.insert(iter, k);
                    break;
                }
            }
            if(temp_k.empty())temp_k.insert(iter, k);
        }
    }
/*
    for(uint32_t i=0;i<temp_k.size();i++)
    {
        std::string str;
        temp_k[i].text.convertString(str, izenelib::util::UString::UTF_8);
        LOG(INFO)<<str<<"  "<<temp_k[i].positions[0].begin<<"  "<<temp_k[i].positions[0].end<<endl;
    }
*/
    boost::unordered_map<std::string, uint32_t> note;
    for(uint32_t i=0;i<temp_k.size();i++)
    {
        KeywordTag& ki = temp_k[i];
        std::string term, str;
        ki.text.convertString(str, izenelib::util::UString::UTF_8);
        term = str;
        uint32_t j = i;
        boost::unordered_map<uint32_t, uint32_t> spus;
        std::vector<AttributeApp>::iterator it1 = ki.attribute_apps.begin();
        bool is_brand = false;
        bool is_model = false;
        if(ki.attribute_apps[0].attribute_name == "品牌")
            is_brand = true;
        else if(ki.attribute_apps[0].attribute_name == "型号")
            is_model = true;

        uint32_t end_pos = ki.positions[0].end;
        if(is_brand)
        {
            for(uint32_t i=0;i<ki.attribute_apps.size();i++)
            {
                spus[ki.attribute_apps[i].spu_id] = 1;
            }
        }
        if(is_brand)
        while(j<temp_k.size()-1)
        {
            j++;
            if(temp_k[j].positions[0].begin > temp_k[i].positions[0].end + 1)
            {
                break;
            }
            bool has_space = false;
            bool has_mid = false;
            bool has_mid2 = false;
            if(temp_k[j].attribute_apps[0].attribute_name == "品牌")
            {
                std::string str;
                term_list[temp_k[i].positions[0].end].text.convertString(str, izenelib::util::UString::UTF_8);
                if(str.compare("/") == 0)
                    has_mid = true;
//                else break;
            }
            
            if(temp_k[j].positions[0].begin == temp_k[i].positions[0].end + 1)
            {
                if(term_list[temp_k[i].positions[0].end].position == 0)
                    has_space = true;
                else if(!has_mid) 
                {
                    std::string str;
                    term_list[temp_k[i].positions[0].end].text.convertString(str, izenelib::util::UString::UTF_8);
                    if(str.compare("-") == 0)
                       has_mid2 = true;
                    else break;
                }
            }
            
            boost::unordered_map<uint32_t, uint32_t> sp, sp2;
            std::vector<AttributeApp>::iterator it2 = temp_k[j].attribute_apps.begin();
            
            sp2 = spus;
            while(it2!=temp_k[j].attribute_apps.end())
            {
                if(spus.find(it2->spu_id) == spus.end())
                {
                    sp2[it2->spu_id] = 1;
                    it2++;
                    continue;
                }
                sp[it2->spu_id]=1;
 //               break;
                it2++;
            }
                  
            if(sp.size() == 0)
            {
                if(temp_k[j].positions[0].begin <= temp_k[i].positions[0].end)
                    i++;
                break;
            }
            spus.clear();
            if(has_mid)spus = sp2;
            else spus = sp;
            LOG(INFO)<<"spus size: "<<spus.size()<<endl;
            
            temp_k[j].text.convertString(str,izenelib::util::UString::UTF_8);
           
            if(has_space)
                term += " "+str;
            else if(has_mid)
                term += "/"+str;
            else if(has_mid2)
                term += "-" + str;
            else
                term += str;
            i++;
            end_pos = temp_k[j].positions[0].end;
        }
//        LOG(INFO)<<"Term res: "<<term<<"  "<<temp_k[i].positions[0].begin<<endl;

        if(is_brand)
        {
            bool has_space = false;
            bool has_num = false;
            bool has_cha = false;
            std::string str = "";
            while(end_pos<term_list.size())
            {
                std::string s;
                term_list[end_pos].text.convertString(s, izenelib::util::UString::UTF_8);
                if(term_list[end_pos].position == 0)
                {
                    end_pos++;
                    has_space = true;
                    continue;
                }
                if(s.at(0)>='a' && s.at(0)<='z')
                {
                    if(s.at(s.size()-1) >='0' && s.at(s.size()-1) <='9')
                        has_num = true;
                    if(has_space)
                        str +=" "+s;
                    else str += s;
                    has_space = false;
                    has_cha = true;
                    end_pos++;
                }
                else if(s.at(0)>='0' && s.at(0)<='9')
                {
                    if(has_space)
                        str +=" "+s;
                    else str += s;
                    has_space = false;
                    has_num = true;
                    end_pos++;
                }
                else if(s.compare("-") == 0)
                {
                    str += s;
                    end_pos++;
                }
                else break;
            }
            std::string ss;
            ki.text.convertString(ss, izenelib::util::UString::UTF_8);
            if(term.compare(ss) != 0)term += str;
            else if(has_num && has_cha) term+=str;
        }
        if(note.find(term) == note.end())
        {
            ki.text.convertString(str, izenelib::util::UString::UTF_8);
            if(term.compare(str)!=0 || is_brand)
            {
                uint32_t weight;
                if(term.compare(str) != 0)
                    weight = 10;
                else weight = 5;
                note[term]=1;
                UString usterm;
                usterm.assign(term, izenelib::util::UString::UTF_8);
                res_brand.push_back(std::make_pair(usterm, std::make_pair(ki.positions[0].begin, weight)));
            }
            else if(is_model)
            {
                for(uint32_t m=0;m<term.size();m++)
                {
                    if(term.at(m)>='a' && term.at(m)<='z')
                    {
                        uint32_t weight;
                        if(term.length() > 10)
                            weight = 8;
                        else weight = 4;
                        note[term]=1;
                        UString usterm;
                        usterm.assign(term, izenelib::util::UString::UTF_8);
                        res_model.push_back(std::make_pair(usterm, std::make_pair(ki.positions[0].begin, weight)));
                        break;
                    }
                }
            }
            else if(term.length() > 16)
            {
                UString usterm;
                usterm.assign(term, izenelib::util::UString::UTF_8);
                res_model.push_back(std::make_pair(usterm, std::make_pair(ki.positions[0].begin, 7)));
            }
        }
    }
}
void ProductMatcher::ExtractKeywordsFromPage(const UString& text, std::map<std::string, std::vector<std::pair<UString, uint32_t> > > & res, std::list<std::pair<UString, uint32_t> > & category, std::map<UString, uint32_t>& weight)
{
    if(!IsOpen()) return;
    if(text.length() == 0) return;
    
    ATermList term_list;
    Analyze_(text, term_list);

    KeywordVector keyword_vector;
    GetKeywords(term_list, keyword_vector, false);
    
    FeatureVector feature_vector;
    GenFeatureVector_(keyword_vector, feature_vector);
    boost::unordered_map<cid_t, uint32_t>::iterator iter;
    
    std::vector<std::pair<cid_t, double> > cos_value;
    for(iter=first_level_category_.begin();iter!=first_level_category_.end();iter++)
    {
        double tcos = Cosine_(feature_vector, feature_vectors_[iter->first]);
        std::vector<std::pair<cid_t, double> >::iterator it;
        for(it = cos_value.begin();it!=cos_value.end();it++)
        {
            if(it->second < tcos)
                break;
        }
        cos_value.insert(it, make_pair(iter->first, tcos));
    }
    for(uint32_t i=0;i<10;i++)
    {
        LOG(INFO)<<"i: "<<i<<" cos: "<<cos_value[i].second<<" cid: "<<cos_value[i].first<<" name: "<<category_list_[cos_value[i].first].name<<endl;
    }

    boost::unordered_map<std::string, uint32_t> note;
    KeywordVector::iterator it;
    for(it = keyword_vector.begin();it!=keyword_vector.end();it++)
    {
        std::string term;
        it->text.convertString(term, izenelib::util::UString::UTF_8);
        if(it->category_name_apps.size()>0)
        {
            if(note.find(term) ==note.end())
             {
                 std::vector<CategoryNameApp>::iterator iter = it->category_name_apps.begin();
                 bool is_res = false;
                 for(;iter!=it->category_name_apps.end();iter++)
                 {
/*
                     cout<<"term: " <<term
                       <<" cid: "<<iter->cid
                       <<" depth: "<<iter->depth
                       <<" is_complete: "<<iter->is_complete
                       <<" category: "<< category_list_[iter->cid].name
                       <<" parent_cid: " <<category_list_[iter->cid].parent_cid
                       <<" is_parent: " <<category_list_[iter->cid].is_parent
                       <<" depth: " <<category_list_[iter->cid].depth
                       <<" has_spu: " << category_list_[iter->cid].has_spu<<endl;
*/
                     std::string man_category = category_list_[iter->cid].name.substr(0,category_list_[iter->cid].name.find_first_of(">"));
                     if(man_category.find(term) != std::string::npos)
                         is_res = true;
                 }
                 if(is_res)
                 {
                     category.push_back(std::make_pair(it->text, it->positions[0].begin));
                     note[term]=1;
                 }
             }
        }
        else if(it->attribute_apps.size() > 0)
        {
            std::vector<AttributeApp>::iterator iter = it->attribute_apps.begin();
            
            for(uint32_t i=0;iter!=it->attribute_apps.end();iter++,i++)
            {
                if(i>10 || it->attribute_apps.size() < 15)break;
                if(iter->attribute_name !="品牌" && iter->attribute_name != "型号")
                    continue;
		cid_t cid = products_[iter->spu_id].cid;
                cid = GetLevelCid_(category_list_[cid].name, 1);
                bool b = false;
                for(uint32_t i=0;i<5;i++)
                {
		    if(cos_value[i].first == cid)
                    {
                        b = true;
                        break;
                    }
                }
                if(!b)continue;
                
                std::string pt = products_[iter->spu_id].stitle;
                if(res.find(pt) == res.end())
                {
                    std::vector<std::pair<UString, uint32_t> > v;
                    res[pt] = v;
                }
                weight[it->text] = it->attribute_apps.size();
                std::string value =term;
                UString uterm;
                uterm.assign(value, izenelib::util::UString::UTF_8);
                for(uint32_t i=0;i<it->positions.size();i++)
                    res[pt].push_back(std::make_pair(it->text, it->positions[i].begin));
            }
        }
    }
}
void ProductMatcher::GetSearchKeywords(const UString& text, std::list<std::pair<UString, double> >& hits, std::list<std::pair<UString, double> >& left_hits, std::list<UString>& left)
{
    if(!IsOpen()) return;
    if(text.length()==0) return;
#ifdef B5M_DEBUG
    izenelib::util::ClockTimer clocker;
#endif
    static const double category_weight = 4.0;
    static const double brand_weight = 4.0;
    static const double type_weight = 3.0;
    static const double thirdparty_weight = 4.0;
    static const double left_weight = 1.0;
    ATermList term_list;
    Analyze_(text, term_list);
    KeywordVector keyword_vector;
    GetKeywords(term_list, keyword_vector, false);
    Document doc;
    doc.property("Title") = ustr_to_propstr(text);
    std::vector<Product> result_products;
#ifdef B5M_DEBUGgi
    std::cout<<"[BEFORE COMPUTE]"<<clocker.elapsed()<<std::endl;
    clocker.restart();
#endif
    Compute2_(doc, term_list, keyword_vector, 1, result_products);
#ifdef B5M_DEBUG
    std::cout<<"[COMPUTE]"<<clocker.elapsed()<<std::endl;
    clocker.restart();
#endif
    //typedef boost::unordered_set<TermList> Strict;
    typedef boost::unordered_set<std::string> Strict;
    typedef boost::unordered_map<std::string,UString> StrictMap;
    Strict strict;
    bool spu_matched = false;
    if(result_products.size()==1&&!result_products[0].spid.empty()&&!result_products[0].stitle.empty())
    {
        spu_matched = true;
    }
    if(spu_matched)//is spu matched
    {
        StrictMap map;
        const Product& p = result_products.front();
        for(uint32_t a = 0;a<p.attributes.size();a++)
        {
            const Attribute& attr = p.attributes[a];
            if(attr.is_optional) continue;
            for(uint32_t v=0;v<attr.values.size();v++)
            {
                std::string terms_str;
                UString text(attr.values[v], UString::UTF_8);
                GetTermsString_(text, terms_str);
                strict.insert(terms_str);
                map.insert(std::make_pair(terms_str, text));
#ifdef B5M_DEBUG
                LOG(INFO)<<"spu keyword "<<attr.values[v]<<","<<terms_str<<std::endl;
#endif
                //TermList term_list;
                //GetTerms_(attr.values[v], term_list);
                //strict.insert(term_list);
            }
        }
        for(uint32_t i=0;i<keyword_vector.size();i++)
        {
            KeywordTag& ki = keyword_vector[i];
            std::string terms_str;
            GetTermsString_(ki.text, terms_str);
            StrictMap::const_iterator it = map.find(terms_str);
            if(it!=map.end())
            {
                ki.text = it->second;
                ki.kweight=1.0;
            }
        }

    }
#ifdef B5M_DEBUG
    std::cout<<"[SPU]"<<clocker.elapsed()<<std::endl;
    clocker.restart();
#endif
    //do attribute synomym filter
    for(uint32_t i=0;i<keyword_vector.size();i++)
    {
        KeywordTag& ki = keyword_vector[i];
        if(ki.kweight<=0.0) continue;
        for(uint32_t j=i+1;j<keyword_vector.size();j++)
        {
            KeywordTag& kj = keyword_vector[j];
            if(kj.kweight<=0.0) continue;
            if(ki.IsAttribSynonym(kj))
            {
                if(ki.kweight!=kj.kweight)
                {
                    if(ki.kweight<kj.kweight)
                    {
                        ki.kweight = 0.0;
#ifdef B5M_DEBUG
                        LOG(INFO)<<"keyword "<<i<<" set to 0"<<std::endl;
#endif
                    }
                    else
                    {
#ifdef B5M_DEBUG
                        LOG(INFO)<<"keyword "<<j<<" set to 0"<<std::endl;
#endif
                        kj.kweight = 0.0;
                    }
                }
                else
                {
                    if(ki.text.length()==0) ki.kweight=0.0;
                    else if(kj.text.length()==0) kj.kweight=0.0;
                    else
                    {
                        bool cni = ki.text.isChineseChar(0);
                        bool cnj = kj.text.isChineseChar(0);
                        if(!cni&&cnj)
                        {
#ifdef B5M_DEBUG
                            LOG(INFO)<<"keyword "<<i<<" set to 0"<<std::endl;
#endif
                            ki.kweight = 0.0;
                        }
                        else
                        {
#ifdef B5M_DEBUG
                            LOG(INFO)<<"keyword "<<j<<" set to 0"<<std::endl;
#endif
                            kj.kweight = 0.0;
                        }
                    }
                }
            }
        }
    }

#ifdef B5M_DEBUG
    std::cout<<"[SYN]"<<clocker.elapsed()<<std::endl;
    clocker.restart();
#endif
    if(!spu_matched)//not spu matched
    {
        std::string pattern_type;
        std::string stext;
        text.convertString(stext, UString::UTF_8);
        boost::algorithm::to_lower(stext);
        //std::cerr<<"stext "<<stext<<std::endl;
        boost::sregex_token_iterator iter(stext.begin(), stext.end(), type_regex_, 0);
        boost::sregex_token_iterator end;
        for( ; iter!=end; ++iter)
        {
            const std::string& candidate = *iter;
            if(candidate[0]=='-' || candidate[candidate.length()-1]=='-') continue;
            bool has_digit = false;
            for(uint32_t i=0;i<candidate.length();i++)
            {
                char c = candidate[i];
                if(c>='0'&&c<='9')
                {
                    has_digit = true;
                    break;
                }
            }
            if(!has_digit) continue;
            if(candidate.length()>pattern_type.length())
            {
                pattern_type = candidate;
            }
        }
        if(!pattern_type.empty())
        {
            for(uint32_t i=0;i<keyword_vector.size();i++)
            {
                KeywordTag& k = keyword_vector[i];
                std::string str;
                k.text.convertString(str, UString::UTF_8);

                if(pattern_type==str)
                {
                    pattern_type.clear();
                    break;
                }
                else if(pattern_type.find(str)!=std::string::npos&&pattern_type.length()>str.length())
                {
                    k.kweight = 0.0;
                }
                else if(str.find(pattern_type)!=std::string::npos&&str.length()>pattern_type.length())
                {
                    pattern_type.clear();
                    break;
                }
            }
        }
        if(!pattern_type.empty())
        {
            KeywordTag new_k;
            new_k.text = UString(pattern_type, UString::UTF_8);
            new_k.kweight = 1.0;
            AttributeApp app;
            app.spu_id = 0;//virtual
            app.attribute_name = "型号";
            app.is_optional = false;
            new_k.attribute_apps.push_back(app);
            //add position
            std::string low_pattern = boost::algorithm::to_lower_copy(pattern_type);
            std::size_t index = 0;
            Position position;
            for(uint32_t t=0;t<term_list.size();t++)
            {
                std::string str = term_list[t].TextString();
                std::size_t left_len = low_pattern.length()-index;
                if(str.length()>left_len)
                {
                    index=0;
                    continue;
                }
                bool is_start = index==0? true : false;
                bool match = true;
                for(std::size_t i=0;i<str.length();i++)
                {
                    if(str[i]!=low_pattern[index++])
                    {
                        match = false;
                        break;
                    }
                }
                if(!match)
                {
                    index = 0;
                }
                else
                {
                    if(is_start) position.begin = t;
                    if(index==low_pattern.length())
                    {
                        position.end = t+1;
                        new_k.positions.push_back(position);
                        break;
                    }
                }
            }

            keyword_vector.push_back(new_k);
        }
        //std::string::const_iterator start = stext.begin();
        //std::string::const_iterator end = stext.end();
        //boost::smatch what;
        //while( boost::regex_search(start, end, what, type_regex_))
        //{
            //std::cerr<<"regex size "<<what.size()<<std::endl;
            ////std::cerr<<what.str()<<std::endl;
            ////std::string tmatch(what[0].first, what[0].second);
            ////std::cerr<<"type matching "<<tmatch<<std::endl;
        //}
    }
#ifdef B5M_DEBUG
    for(uint32_t i=0;i<keyword_vector.size();i++)
    {
        std::string str;
        keyword_vector[i].text.convertString(str, UString::UTF_8);
        LOG(INFO)<<"keyword "<<str<<","<<keyword_vector[i].kweight<<std::endl;
    }
#endif

    typedef boost::dynamic_bitset<> AllStrict;
    AllStrict all_strict(term_list.size());
    for(uint32_t i=0;i<keyword_vector.size();i++)
    {
        KeywordTag& k = keyword_vector[i];
        if(k.kweight<0.0) continue;
        UString original_text;
        if(!k.positions.empty())
        {
            const Position& pos = k.positions.front();
            for(uint32_t b=pos.begin;b<pos.end;b++)
            {
                original_text.append(term_list[b].text);
            }
        }
        if(!original_text.empty())
        {
            k.text = original_text;
        }
        for(uint32_t p=0;p<k.positions.size();p++)
        {
            const Position& pos = k.positions[p];
            for(uint32_t b=pos.begin;b<pos.end;b++)
            {
                all_strict.set(b);
            }
        }
        std::string terms_str;
        GetTermsString_(k.text, terms_str);
        //all_strict.insert(terms_str);
        if(k.kweight<=0.0 || (!strict.empty()&&strict.find(terms_str)==strict.end()) )
        {
            left_hits.push_back(std::make_pair(k.text, left_weight));
#ifdef B5M_DEBUG
            std::string str;
            k.text.convertString(str, UString::UTF_8);
            LOG(INFO)<<"left hit "<<str<<std::endl;
#endif
        }
        else
        {
            bool is_category = false;
            bool is_brand = false;
            bool is_type = false;
            bool is_thirdparty = false;
            if(k.category_name_apps.empty()&&k.attribute_apps.empty())
            {
                is_thirdparty = true;
            }
            if(!k.category_name_apps.empty())
            {
                is_category = true;
            }
            for(uint32_t j=0;j<k.attribute_apps.size();j++)
            {
                const AttributeApp& app = k.attribute_apps[j];
                if(app.attribute_name=="品牌")
                {
                    is_brand = true;
                }
                else if(app.attribute_name=="型号")
                {
                    is_type = true;
                }
            }
            //if(!is_category&&is_type)
            //{

            //}
            double class_weight = 1.0;
            if(is_category) class_weight = category_weight;
            else if(is_brand) class_weight = brand_weight;
            else if(is_type) class_weight = type_weight;
            else if(is_thirdparty) class_weight = thirdparty_weight;
            double kweight = k.kweight;
            kweight*=class_weight;
            //std::string stext;
            //k.text.convertString(stext, UString::UTF_8);
            //std::cerr<<"[HITS]"<<stext<<std::endl;
            hits.push_back(std::make_pair(k.text, kweight));
#ifdef B5M_DEBUG
            std::string str;
            k.text.convertString(str, UString::UTF_8);
            LOG(INFO)<<"hit "<<str<<std::endl;
#endif
        }
    }
    UString left_text;
    bool has_text = false;
    for(uint32_t i=0;i<term_list.size();i++)
    {
        const Term& term = term_list[i];
        if(!all_strict[i])
        {
            left_text.append(term.text);
            if(!IsSymbol_(term))
            {
                has_text = true;
            }
        }
        else
        {
            UString::algo::compact(left_text);
            if(!left_text.empty()&&has_text)
            {
                left.push_back(left_text);
            }
            left_text.clear();
            has_text = false;
        }
    }
    UString::algo::compact(left_text);
    if(!left_text.empty()&&has_text)
    {
        left.push_back(left_text);
#ifdef B5M_DEBUG
        std::string str;
        left_text.convertString(str, UString::UTF_8);
        LOG(INFO)<<"left "<<str<<std::endl;
#endif
    }
}

//void ProductMatcher::SearchKeywordsFilter_(std::vector<KeywordTag>& keywords)
//{
    //std::
//}

void ProductMatcher::GetFuzzyKeywords_(const ATermList& term_list, KeywordVector& keyword_vector, cid_t cid)
{
#ifdef B5M_DEBUG
    //LOG(INFO)<<"process fuzzy on cid "<<cid<<std::endl;
#endif
    //std::vector<TermIndexValue> fuzzy_apps;
    typedef boost::unordered_map<uint32_t, FuzzyApp> FuzzyApps;
    TermIndexMap::const_iterator tim_it = term_index_map_.find(cid);
    if(tim_it==term_index_map_.end())
    {
        return;
    }
#ifdef B5M_DEBUG
    //LOG(INFO)<<"find fuzzy on cid "<<cid<<std::endl;
#endif
    const TermIndex& term_index = tim_it->second;
    FuzzyApps fuzzy_apps;//fuzzy keyword index to fuzzy app
    uint32_t text_pos = 0;
    typedef TermIndex::Invert Invert;
    typedef std::pair<uint32_t, uint32_t> MatchPosition;
    const Invert& invert = term_index.invert;
    for(uint32_t i=0;i<term_list.size();i++)
    {
        const Term& term = term_list[i];
        if(IsSymbol_(term)) continue;
        uint32_t tpos = text_pos++;
        Invert::const_iterator it = invert.find(term.id);
        if(it==invert.end()) continue;
        const TermIndexValue& tiv = it->second;
        for(uint32_t t=0;t<tiv.items.size();t++)
        {
            const TermIndexItem& item = tiv.items[t];
            FuzzyApp& app = fuzzy_apps[item.keyword_index];
            if(app.positions.empty())
            {
                app.positions.push_back(std::make_pair(tpos, item.pos));
            }
            else {
                MatchPosition& mp = app.positions.back();
                if(mp.first!=tpos) app.positions.push_back(std::make_pair(tpos, item.pos));
                else
                {
                    bool mp_kdd = false;
                    bool kdd = false;
                    for(uint32_t i=0;i<app.positions.size()-1;i++)
                    {
                        if(app.positions[i].second==mp.second)
                        {
                            mp_kdd = true;
                            break;
                        }
                    }
                    for(uint32_t i=0;i<app.positions.size()-1;i++)
                    {
                        if(app.positions[i].second==item.pos)
                        {
                            kdd = true;
                            break;
                        }
                    }
                    if(mp_kdd&&!kdd)
                    {
                        mp.second = item.pos;
                    }
                }
            }
            //app.kpos.push_back(item.pos);
            //app.tpos.push_back(tpos);
        }
    }
    for(FuzzyApps::iterator it = fuzzy_apps.begin();it!=fuzzy_apps.end();++it)
    {
        FuzzyApp& app = it->second;
        FuzzyApp& m = app;
        double iavgkstart = 0.0;
        for(uint32_t i=0;i<m.positions.size();i++)
        {
            iavgkstart+=((double)m.positions[i].first-(double)m.positions[i].second);
        }
        if(iavgkstart<0.0) iavgkstart=0.0;
        iavgkstart/=m.positions.size();
        boost::dynamic_bitset<> flag(m.positions.size());
        boost::dynamic_bitset<> dflag(m.positions.size());
        for(uint32_t i=0;i<m.positions.size();i++)
        {
            if(flag[i]) continue;
            std::vector<std::pair<double, uint32_t> > dists;
            const MatchPosition& pos = m.positions[i];
            double kstart = (double)pos.first-(double)pos.second;
            double dist = std::abs(iavgkstart-kstart);
            dists.push_back(std::make_pair(dist, i));
            flag.set(i);
            for(uint32_t j=i+1;j<m.positions.size();j++)
            {
                if(flag[j]) continue;
                const MatchPosition& jpos = m.positions[j];
                if(pos.second==jpos.second)
                {
                    double kstart = (double)jpos.first-(double)jpos.second;
                    double dist = std::abs(iavgkstart-kstart);
                    dists.push_back(std::make_pair(dist, j));
                    flag.set(j);
                }

            }
            if(dists.size()<2) continue;
            std::sort(dists.begin(),dists.end());
            for(uint32_t j=1;j<dists.size();j++)
            {
                dflag.set(dists[j].second);
            }
        }
        if(dflag.count()>0)
        {
            std::vector<MatchPosition> newp;
            newp.reserve(m.positions.size());
            for(uint32_t i=0;i<dflag.size();i++)
            {
                if(!dflag[i])
                {
                    newp.push_back(m.positions[i]);
                }
            }
            m.positions.swap(newp);
        }
        m.ilength = m.positions.back().first - m.positions.front().first+1;
        m.count = m.positions.size();
        //std::sort(app.kpos.begin(), app.kpos.end());
        //app.kpos.erase( std::unique(app.kpos.begin(), app.kpos.end()), app.kpos.end());
        //std::sort(app.tpos.begin(), app.tpos.end());
        //app.tpos.erase( std::unique(app.tpos.begin(), app.tpos.end()), app.tpos.end());
        const ATermList& keyword = term_index.forward[it->first];
        if(IsFuzzyMatched_(keyword, app))
        {
            TermList keyword_id_list(keyword.size());
            for(uint32_t i=0;i<keyword.size();i++)
            {
                keyword_id_list[i] = keyword[i].id;
            }
            bool dd = false;
            for(uint32_t i=0;i<keyword_vector.size();i++)
            {
                const KeywordTag& k = keyword_vector[i];
                if(k.term_list==keyword_id_list)
                {
                    dd = true;
                    break;
                }
            }
            if(!dd)
            {
                TrieType::const_iterator tit = trie_.find(keyword_id_list);
                if(tit!=trie_.end())
                {
                    keyword_vector.push_back(tit->second);
                    keyword_vector.back().kweight = -0.1;
                }
            }
        }
    }
    //std::vector<FuzzyApp> fuzzy_apps;
    //for(uint32_t begin = 0;begin<term_list.size();++begin)
    //{
        //term_t ti = term_list[begin];
        //if(IsSymbol_(ti)) continue;
        //std::vector<term_t> k(1, ti);
        //k.reserve(10);
        //for(uint32_t j=begin+1;j<term_list.size();++j)
        //{
            //term_t tj = term_list[j];
            //if(IsSymbol_(tj)) continue;
            //k.push_back(tj);
            //uint32_t len = k.size();
            ////uint32_t len = end-begin;
            ////std::vector<term_t> k(term_list.begin()+begin, term_list.begin()+end);
            //for(FuzzyTrie::const_iterator it = ftrie_.lower_bound(k); it!=ftrie_.end(); ++it)
            //{
                //if(!boost::algorithm::starts_with(it->first, k)) break;
                //const std::vector<FuzzyApp>& f_app_list = it->second.fuzzy_apps;
                //for(uint32_t f=0;f<f_app_list.size();f++)
                //{
                    //const FuzzyApp& app = f_app_list[f];
                    //fuzzy_apps.push_back(app);
                    //FuzzyApp& push_app = fuzzy_apps.back();
                    ////fuzzy_apps.back().begin = begin;
                    //push_app.pos.end = push_app.pos.begin+len;
                    //push_app.tpos.begin = begin;
                    //push_app.tpos.end = begin+len;
//#ifdef B5M_DEBUG
                    //std::cerr<<"find fuzzy position "<<GetText_(app.term_list)<<","<<push_app.begin<<","<<push_app.end<<std::endl;
//#endif
                //}
            //}
        //}
    //}
    //std::sort(fuzzy_apps.begin(), fuzzy_apps.end(), FuzzyApp::PositionCompare);
    //typedef std::map<TermList, FuzzyPositions> FuzzyPositionMap;
    //FuzzyPositionMap fp_map;
    //for(uint32_t i=0;i<fuzzy_apps.size();i++)
    //{
        //const FuzzyApp& app = fuzzy_apps[i];
//#ifdef B5M_DEBUG
        //std::cerr<<"inserting fuzzy position "<<GetText_(app.term_list)<<","<<app.begin<<","<<app.end<<std::endl;
//#endif
        //FuzzyPositions::iterator it = fuzzy_positions.find(app.term_list);
        //if(it==fuzzy_positions.end())
        //{
//#ifdef B5M_DEBUG
            //std::cerr<<"insert fuzzy position "<<app.begin<<","<<app.end<<std::endl;
//#endif
            //FuzzyPositions fp;
            //fp.positions.push_back(app.pos);
            //fp.tpositions.push_back(app.tpos);
            //fuzzy_positions.insert(std::make_pair(app.term_list, fp));
        //}
        //else
        //{
            //FuzzyPositions& ep = it->second;
            //bool overlap = false;
            //for(uint32_t p=0;p<ep.positions.size();p++)
            //{
                //Position& pos = ep.positions[p];
                //if(pos.Combine(app.pos))
                //{
                    //overlap = true;
                    //break;
                //}
            //}
            //if(!overlap)
            //{
                //ep.positions.push_back(app.pos);
            //}

            //overlap = false;
            //for(uint32_t p=0;p<ep.tpositions.size();p++)
            //{
                //Position& pos = ep.tpositions[p];
                //if(pos.Combine(app.tpos))
                //{
                    //overlap = true;
                    //break;
                //}
            //}
            //if(!overlap)
            //{
                //ep.tpositions.push_back(app.tpos);
            //}
        //}
    //}
    //for(FuzzyPositions::const_iterator it = fuzzy_positions.begin(); it!=fuzzy_positions.end();++it)
    //{
        //const TermList& term_list = it->first;
        //const Positions& positions = it->second;
        //uint32_t begin = positions.front().first;
        //uint32_t end = positions.back().second;
        //double len = end-begin;
        //double max_len = term_list.size()*1.5;
        //if(len<=max_len && len>=term_list.size())
        //{
            //TrieType::const_iterator tit = trie_.find(term_list);
            //if(tit!=trie_.end())
            //{
                //keyword_vector.push_back(tit->second);
//#ifdef B5M_DEBUG
                //std::cerr<<"find fuzzy "<<GetText_(term_list)<<","<<begin<<","<<end<<std::endl;
//#endif
            //}
        //}

    //}

}


bool ProductMatcher::IsFuzzyMatched_(const ATermList& keyword, const FuzzyApp& app) const
{
    bool all_chinese = true;
    for(uint32_t i=0;i<keyword.size();i++)
    {
        const UString& text = keyword[i].text;
        bool cn = false;
        if(text.length()==1&&text.isChineseChar(0))
        {
            cn = true;
        }
        if(!cn)
        {
            all_chinese = false;
            break;
        }
    }
    uint32_t iswap=0;
    std::vector<uint32_t> kpos(app.positions.size());
    for(uint32_t i=0;i<app.positions.size();i++)
    {
        kpos[i] = app.positions[i].second;
    }
    for(uint32_t i=0;i<kpos.size();i++)
    {
        for(uint32_t j=i;j<kpos.size()-1;j++)
        {
            if(kpos[j]>kpos[j+1])
            {
                std::swap(kpos[j], kpos[j+1]);
                iswap++;
            }
        }
    }
    uint32_t keyword_len = keyword.size();
    double swap = (double)iswap/keyword.size();
    double k_ratio = (double)app.count/keyword_len;
    double t_ratio = (double)app.ilength/keyword_len;
    //double k_ratio = (double)app.kpos.size()/keyword.size();
    //double t_ratio = (double)(app.tpos.back()-app.tpos.front())/keyword.size();
#ifdef B5M_DEBUG
    TermList keyword_id_list(keyword.size());
    for(uint32_t i=0;i<keyword.size();i++)
    {
        keyword_id_list[i] = keyword[i].id;
    }
    TrieType::const_iterator tit = trie_.find(keyword_id_list);
    std::string sk;
    if(tit!=trie_.end())
    {
        tit->second.text.convertString(sk, UString::UTF_8);
    }
    LOG(ERROR)<<"[FUZZY] "<<sk<<","<<all_chinese<<","<<swap<<","<<k_ratio<<","<<t_ratio<<std::endl;
#endif
    if(all_chinese)
    {
        if(swap>1.0) return false;
        if(k_ratio>=0.75&&t_ratio<=1.3) return true;
        if(k_ratio>=1.0&&t_ratio<=1.5) return true;
        return false;
    }
    else
    {
        if(k_ratio>=1.0&&t_ratio<=1.5) return true;
        return false;
    }
}

ProductMatcher::cid_t ProductMatcher::GetCid_(const UString& category) const
{
    cid_t cid = 0;
    std::string scategory;
    category.convertString(scategory, UString::UTF_8);
    if(!scategory.empty())
    {
        CategoryIndex::const_iterator it = category_index_.find(scategory);
        if(it!=category_index_.end())
        {
            cid = it->second;
        }
    }
    return cid;
}

uint32_t ProductMatcher::GetCidBySpuId_(uint32_t spu_id)
{
    //LOG(INFO)<<"spu_id:"<<spu_id<<std::endl;
    const Product& product = products_[spu_id];
    const std::string& scategory = product.scategory;
    return category_index_[scategory];
}

uint32_t ProductMatcher::GetCidByMaxDepth_(uint32_t cid)
{
    while(true)
    {
        const Category& c = category_list_[cid];
        if(c.depth<=category_max_depth_)
        {
            return cid;
        }
        cid = c.parent_cid;
    }
    return 0;
}

bool ProductMatcher::EqualOrIsParent_(uint32_t parent, uint32_t child) const
{
    uint32_t cid = child;
    while(true)
    {
        if(cid==0) return false;
        if(cid==parent) return true;
        const Category& c = category_list_[cid];
        cid = c.parent_cid;
    }
    return false;
}

void ProductMatcher::GenCategoryContributor_(const KeywordTag& tag, CategoryContributor& cc)
{
    double kweight = tag.kweight;
    if(kweight<=0.0) return;
    double all_score = 0.0;
#ifdef B5M_DEBUG
    //std::string stext;
    //tag.text.convertString(stext, UString::UTF_8);
    //std::cout<<"[TAGTEXT]"<<stext<<std::endl;
#endif

    uint32_t category_name_count=0;
    uint32_t brand_count = 0;
    uint32_t model_count = 0;
    uint32_t optional_count = 0;
    CategoryContributor ccc;
    for(uint32_t i=0;i<tag.category_name_apps.size();i++)
    {
        const CategoryNameApp& app = tag.category_name_apps[i];
        category_name_count++;
        //if(matcher_only)
        //{
            //if(!EqualOrIsParent_(given_cid, app.cid)) continue;
        //}
        double depth_ratio = 1.0-0.2*(app.depth-1);
        double times = 1.0;
        //if(count_weight>=0.8) times = 2.5;
        //else if(count_weight>=0.5) times = 2.0;
        double share_point = times*depth_ratio;
        if(!app.is_complete) share_point*=0.6;
        double add_score = share_point;
        CategoryContributor::iterator it = ccc.find(app.cid);
        if(it==ccc.end())
        {
            ccc.insert(std::make_pair(app.cid, add_score));
        }
        else if(add_score>it->second)
        {
            it->second = add_score;
        }
#ifdef B5M_DEBUG
        //std::cout<<"[CNA]"<<category_list_[app.cid].name<<","<<app.depth<<","<<app.is_complete<<","<<add_score<<std::endl;
#endif
    }
    CategoryContributor acc;
    for(uint32_t i=0;i<tag.attribute_apps.size();i++)
    {
        const AttributeApp& app = tag.attribute_apps[i];
        if(app.spu_id==0) continue;
        const Product& p = products_[app.spu_id];
        double share_point = 0.0;
        double p_point = 0.0;
        if(app.is_optional)
        {
            share_point = 0.1;
            p_point = 0.1;
            optional_count++;
        }
        else if(app.attribute_name=="型号")
        {
            share_point = 0.3;
            p_point = 1.5;
            model_count++;
        }
        else if(app.attribute_name=="品牌")
        {
            brand_count++;
            share_point = 0.2;
            p_point = 1.0;
        }
        else
        {
            share_point = 0.2;
            p_point = 1.0;
        }
        double add_score = share_point;
        CategoryContributor::iterator it = acc.find(p.cid);
        if(it==acc.end())
        {
            acc.insert(std::make_pair(p.cid, add_score));
        }
        else
        {
            it->second += add_score;
        }
    }
    for(CategoryContributor::const_iterator it = acc.begin();it!=acc.end();it++)
    {
        double score = it->second;
        if(score>0.4) score = 0.4;
        CategoryContributor::iterator it2 = cc.find(it->first);
        if(it2==cc.end())
        {
            cc.insert(std::make_pair(it->first, score));
        }
        else
        {
            it2->second += score;
        }
        all_score+=score;
#ifdef B5M_DEBUG
        //std::cout<<"[AN]"<<category_list_[it->first].name<<","<<score<<std::endl;
#endif

    }
    for(CategoryContributor::const_iterator it = ccc.begin();it!=ccc.end();it++)
    {
        CategoryContributor::iterator it2 = cc.find(it->first);
        double score = it->second;
        if(it2==cc.end())
        {
            cc.insert(std::make_pair(it->first, score));
        }
        else
        {
            it2->second += score;
        }
    }
    CategoryContributor occ;
    for(uint32_t i=0;i<tag.offer_category_apps.size();i++)
    {
        const OfferCategoryApp& app = tag.offer_category_apps[i];
        //if(matcher_only)
        //{
            //if(!EqualOrIsParent_(given_cid, app.cid)) continue;
        //}
        uint32_t app_count = app.count;
        if(app_count>20) app_count = 20;
        double depth_ratio = 1.0;
        double times = app_count*0.05;
        //if(count_weight>=0.8) times *= 2.5;
        //else if(count_weight>=0.5) times *= 2.0;
        double share_point = times*depth_ratio;
        double add_score = share_point;
        CategoryContributor::iterator it = occ.find(app.cid);
        if(it==occ.end())
        {
            occ.insert(std::make_pair(app.cid, add_score));
        }
        else
        {
            it->second += add_score;
        }
        //all_score+=add_score;
    }
    for(CategoryContributor::const_iterator it = occ.begin();it!=occ.end();it++)
    {
        CategoryContributor::iterator it2 = cc.find(it->first);
        double score = it->second;
        if(it2==cc.end())
        {
            //score/=2.0;
            cc.insert(std::make_pair(it->first, score));
        }
        else
        {
            it2->second += score;
        }
        all_score+=score;
#ifdef B5M_DEBUG
        //std::cout<<"[CNO]"<<category_list_[it->first].name<<","<<score<<std::endl;
#endif
    }
    double inner_kweight = 1.0;
    if(category_name_count==0&& brand_count==0 && model_count==0 && optional_count>=5)
    {
        inner_kweight = 0.3;
    }
#ifdef B5M_DEBUG
    //std::cout<<"[INNERK]"<<inner_kweight<<std::endl;
#endif
    if(all_score>0.0)
    {
        double divide = all_score;
        //if(category_name_count>=2)
        //{
            //divide /= 1.8;
        //}
        for(CategoryContributor::iterator it = cc.begin();it!=cc.end();++it)
        {
            it->second*=(inner_kweight/divide)*kweight;
            //it->second*=inner_kweight*kweight;
#ifdef B5M_DEBUG
            //std::cout<<"[ALLN]"<<category_list_[it->first].name<<","<<it->second<<std::endl;
#endif
        }
    }
}

void ProductMatcher::MergeCategoryContributor_(CategoryContributor& cc, const CategoryContributor& cc2)
{
    //static double default_score = 0.001;
    //for(CategoryContributor::const_iterator it = cc2.begin();it!=cc2.end();it++)
    //{
        //CategoryContributor::iterator it1 = cc.find(it->first);
        //if(it1==cc.end())
        //{
            //cc.insert(std::make_pair(it->first, it->second*default_score));
        //}
        //else
        //{
            //it1->second*=it->second;
        //}
    //}
    for(CategoryContributor::const_iterator it = cc2.begin();it!=cc2.end();it++)
    {
        CategoryContributor::iterator it1 = cc.find(it->first);
        if(it1==cc.end())
        {
            cc.insert(std::make_pair(it->first, it->second));
        }
        else
        {
            it1->second+=it->second;
        }
    }
}

void ProductMatcher::GenSpuContributor_(const KeywordTag& tag, SpuContributor& sc)
{
#ifdef B5M_DEBUG
    //std::string stext;
    //tag.text.convertString(stext, UString::UTF_8);
    //std::cout<<"[TAGTEXT]"<<stext<<std::endl;
#endif

    for(uint32_t i=0;i<tag.attribute_apps.size();i++)
    {
        const AttributeApp& app = tag.attribute_apps[i];
        if(app.spu_id==0) continue;
        SpuContributorValue& scv = sc[app.spu_id];
        boost::unordered_map<std::string, double>::iterator alit = scv.attribute_lenweight.find(app.attribute_name);
        double lenweight = tag.term_list.size();
        if(alit!=scv.attribute_lenweight.end())
        {
            if(lenweight>alit->second) alit->second = lenweight;
            continue;
        }
        //if(scv.IsAttributeFound(app.attribute_name)) continue;
        scv.attribute_lenweight.insert(std::make_pair(app.attribute_name, lenweight));
        double p_point = 0.0;
        if(app.is_optional)
        {
            p_point = 0.1;
        }
        else if(app.attribute_name=="型号")
        {
            p_point = 1.5;
            scv.model_match = true;
        }
        else if(app.attribute_name=="品牌")
        {
            scv.brand_match = true;
            p_point = 1.0;
        }
        else
        {
            p_point = 1.0;
        }
        double paweight = p_point;
        scv.paweight+=paweight;
        //scv.lenweight+=tag.term_list.size();

        //const Product& p = products_[app.spu_id];
        //std::string stext;
        //tag.text.convertString(stext, UString::UTF_8);
        //LOG(ERROR)<<"paweight add "<<stext<<","<<p.stitle<<","<<paweight<<std::endl;
        //LOG(ERROR)<<"lenweight add "<<stext<<","<<p.stitle<<","<<scv.lenweight<<","<<tag.term_list.size()<<std::endl;
    }
}
void ProductMatcher::ComputeT_(const Document& doc, const std::vector<Term>& term_list, KeywordVector& keywords, uint32_t limit, std::vector<Product>& result_products)
{
    //UString title;
    //doc.getProperty("Title", title);
    //std::string stitle;
    //title.convertString(stitle, UString::UTF_8);
//#ifdef B5M_DEBUG
    //std::cout<<"[TITLE]"<<stitle<<std::endl;
//#endif
    //double price = 0.0;
    //UString uprice;
    //if(doc.getProperty("Price", uprice))
    //{
        //ProductPrice pp;
        //pp.Parse(uprice);
        //pp.GetMid(price);
    //}
    //CategoryContributor all_cc;
    //for(KeywordVector::iterator it = keywords.begin();it!=keywords.end();++it)
    //{
        //KeywordTag& tag = *it;
        ////double kweight = tag.kweight;
        ////double terms_length = tl.size()-(tag.ngram-1);
        ////double len_weight = terms_length/text_term_len;
//#ifdef B5M_DEBUG
        //const TermList& tl = it->term_list;
        //std::string text = GetText_(tl);
        //std::cout<<"[KEYWORD]"<<text<<","<<tag.kweight<<std::endl;
//#endif
        //GenContributor_(tag);
        //MergeContributor_(all_cc, tag.cc);
    //}
    //std::vector<std::pair<double, std::string> > candidates;
    //for(CategoryContributor::const_iterator it=all_cc.begin();it!=all_cc.end();++it)
    //{
        //candidates.push_back(std::make_pair(it->second, category_list_[it->first].name));
    //}
    //std::sort(candidates.begin(), candidates.end(), std::greater<std::pair<double, std::string> >());
    //std::size_t clen = std::min(5ul, candidates.size());
    //for(std::size_t i=0;i<clen;i++)
    //{
        //std::cout<<candidates[i].second<<":"<<candidates[i].first<<std::endl;
    //}

}

void ProductMatcher::Compute2_(const Document& doc, const std::vector<Term>& term_list, KeywordVector& keywords, uint32_t limit, std::vector<Product>& result_products)
{
    std::string stitle;
    doc.getString("Title", stitle);
#ifdef B5M_DEBUG
    std::cout<<"[TITLE]"<<stitle<<std::endl;
#endif
    ProductPrice price;
    Document::doc_prop_value_strtype uprice;
    if(doc.getProperty("Price", uprice))
    {
        price.Parse(propstr_to_ustr(uprice));
    }
    uint32_t given_cid = 0;
    std::string sgiven_category;
    doc.getString("Category", sgiven_category);
    if(!sgiven_category.empty())
    {
        CategoryIndex::const_iterator it = category_index_.find(sgiven_category);
        if(it!=category_index_.end())
        {
            given_cid = it->second;
        }
    }
    CategoryContributor all_cc;
    std::vector<CategoryContributor> ccs(keywords.size());
    SpuContributor spu_cc;
    std::string found_brand;
    for(uint32_t i=0;i<keywords.size();i++)
    {
        const KeywordTag& tag = keywords[i];
        std::string str;
        tag.text.convertString(str, UString::UTF_8);
        //double kweight = tag.kweight;
        //double terms_length = tl.size()-(tag.ngram-1);
        //double len_weight = terms_length/text_term_len;
#ifdef B5M_DEBUG
        std::cout<<"[KEYWORD]"<<str<<","<<tag.kweight<<std::endl;
#endif
        if(given_cid==0&&tag.kweight>0.0)
        {
            GenCategoryContributor_(tag,  ccs[i]);
            MergeCategoryContributor_(all_cc, ccs[i]);
        }
        GenSpuContributor_(tag, spu_cc);
        if(price.Positive())
        {
            if(boost::regex_match(str, vol_regex_))
            {
                std::string new_str;
                if(boost::algorithm::ends_with(str, "b"))
                {
                    new_str = str.substr(0, str.length()-1);
                }
                else
                {
                    new_str = str+"b";
                }
#ifdef B5M_DEBUG
                LOG(INFO)<<"find new vol str "<<new_str<<std::endl;
#endif
                UString new_text(new_str, UString::UTF_8);
                KeywordTag new_tag;
                if(GetKeyword(new_text, new_tag))
                {
                    GenSpuContributor_(new_tag, spu_cc);
                }
            }

        }
    }
    std::vector<uint32_t> cid_list;
    std::vector<double> cid_score_list;
    if(given_cid==0)
    {
        std::vector<std::pair<double, cid_t> > candidates;
        for(CategoryContributor::const_iterator it=all_cc.begin();it!=all_cc.end();++it)
        {
            candidates.push_back(std::make_pair(it->second, it->first));
        }
        std::sort(candidates.begin(), candidates.end(), std::greater<std::pair<double, cid_t> >());
        std::size_t clen = std::min(3ul, candidates.size());
        candidates.resize(clen);
        for(std::size_t i=0;i<clen;i++)
        {
            cid_t cid = candidates[i].second;
            const Category& c = category_list_[cid];
            bool kcategory = false;
            if(c.depth==1) //if important category
            {
                kcategory = true;
            }
            double ssim = string_similarity_.Sim(category_list_[cid].name, stitle);
            if(kcategory)
            {
                candidates[i].first *= 1.1;
            }
            candidates[i].first *= (ssim/10.0)+1.0;
        }
        std::sort(candidates.begin(), candidates.end(), std::greater<std::pair<double, cid_t> >());
#ifdef B5M_DEBUG
        //for(std::size_t i=0;i<clen;i++)
        //{
            //std::cout<<category_list_[candidates[i].second].name<<":"<<candidates[i].first<<std::endl;
        //}
#endif
        for(uint32_t i=0;i<clen;i++)
        {
            cid_t cid = candidates[i].second;
            cid_list.push_back(cid);
            cid_score_list.push_back(candidates[i].first);
        }
    }
    else
    {
        cid_list.push_back(given_cid);
        cid_score_list.push_back(1.0);
    }
    //bool matcher_only = matcher_only_;
    std::size_t text_term_len = 0;
    for(uint32_t i=0;i<term_list.size();i++)
    {
        if(!IsSymbol_(term_list[i]))
        {
            ++text_term_len;
        }
    }
    if(cid_list.empty()) return;
    //do vsm filter
    if(given_cid==0)
    {
        FeatureVector feature_vector;
        GenFeatureVector_(keywords, feature_vector);
        //std::vector<double> cosine_list(cid_list.size());
        for(uint32_t i=0;i<cid_list.size();i++)
        {
            cid_t cid = cid_list[i];
            const std::string& scategory = category_list_[cid].name;
            cid_t cid1 = GetLevelCid_(scategory, 1);
            cid_t cid2 = GetLevelCid_(scategory, 2);
            //double cosine1 = 0.0;
            //double cosine2 = 0.0;
            //if(cid1>0) cosine1 = Cosine_(feature_vector, feature_vectors_[cid1]);
            //if(cid2>0) cosine2 = Cosine_(feature_vector, feature_vectors_[cid2]);
            double cosine = 0.0;
            if(cid2>0) cosine = Cosine_(feature_vector, feature_vectors_[cid2]);
            else if(cid1>0) cosine = Cosine_(feature_vector, feature_vectors_[cid1]);
            //cosine_list[i] = cosine;
            cid_score_list[i] = cosine;
#ifdef B5M_DEBUG
            //std::cerr<<"vsm cosine "<<category_list_[cid].name<<" : "<<cosine1<<","<<cosine2<<std::endl;
            std::cerr<<"vsm cosine "<<category_list_[cid].name<<" : "<<cosine<<std::endl;
#endif
        }
        double top_cosine = cid_score_list.front();
        if(top_cosine<0.06)
        {
            uint32_t replace_index = 0;
            double replace_ratio = 0.0;
            for(uint32_t i=1;i<cid_list.size();i++)
            {
                double cosine = cid_score_list[i];
                double ratio = cosine/top_cosine;
                bool b = false;
                if(cosine>=0.1&&ratio>=6.0) b=true;
                else if(cosine>=0.05&&ratio>=10.0) b=true;
                if(b&&ratio>replace_ratio)
                {
                    replace_index = i;
                    replace_ratio = ratio;
                }
            }
            if(replace_index>0)
            {
#ifdef B5M_DEBUG
                std::cerr<<"vsm change "<<category_list_[cid_list[0]].name<<" to "<<category_list_[cid_list[replace_index]].name<<std::endl;
#endif
                std::swap(cid_list[0], cid_list[replace_index]);
                std::swap(cid_score_list[0], cid_score_list[replace_index]);
            }
            else
            {
                if(top_cosine<0.01)
                {
                    cid_list.clear();
                    cid_score_list.clear();
                }
            }
        }
        if(cid_list.empty()) return;
    }
    if(given_cid==0)
    {
        cid_t top_cid = cid_list.front();
        double top_cid_weight = 0.0;
        {
            CategoryContributor::const_iterator it = all_cc.find(top_cid);
            if(it!=all_cc.end())
            {
                top_cid_weight = it->second;
            }
        }
        if(top_cid_weight>0.0)
        {
            std::vector<uint32_t> remove_list;
            uint32_t remove_valid_count = 0;
            for(uint32_t i=0;i<ccs.size();i++)
            {
                const CategoryContributor& cc = ccs[i];
                KeywordTag& keyword = keywords[i];
                if(keyword.kweight>0.0)
                {
                    remove_valid_count++;
                    CategoryContributor::const_iterator it = cc.find(top_cid);
                    double weight = 0.0;
                    if(it!=cc.end())
                    {
                        weight = it->second;
                    }
                    double ratio = weight/top_cid_weight;
                    bool can_remove = true;
                    if(!keyword.category_name_apps.empty())
                    {
                        can_remove = false;
                    }
                    else
                    {
                        uint32_t optional_count=0;
                        uint32_t vol_count=0;
                        for(uint32_t a=0;a<keyword.attribute_apps.size();a++)
                        {
                            const AttributeApp& app = keyword.attribute_apps[a];
                            if(app.is_optional)
                            {
                                optional_count++;
                            }
                            if(app.attribute_name=="容量")
                            {
                                vol_count++;
                            }
                        }
                        double optional_ratio = (double)optional_count/keyword.attribute_apps.size();
                        if(optional_ratio<0.5||vol_count>=2)
                        {
                            can_remove = false;
                        }
                    }
                    if(ratio<0.01&&can_remove)
                    {
                        remove_list.push_back(i);
#ifdef B5M_DEBUG
                        std::string str;
                        keyword.text.convertString(str, UString::UTF_8);
                        LOG(INFO)<<"candidate keyword remove "<<str<<","<<ratio<<std::endl;
#endif
                    }
                }
            }
            double remove_ratio = (double)remove_list.size()/remove_valid_count;
            if(remove_ratio<=0.2)
            {
                for(uint32_t i=0;i<remove_list.size();i++)
                {
                    KeywordTag& keyword = keywords[remove_list[i]];
                    keyword.kweight = -1.0;
#ifdef B5M_DEBUG
                    std::string str;
                    keyword.text.convertString(str, UString::UTF_8);
                    LOG(INFO)<<"keyword remove "<<str<<std::endl;
#endif

                }
            }
        }
    }
    std::vector<SpuMatchCandidate> spu_match_candidates;
    for(SpuContributor::iterator it = spu_cc.begin();it!=spu_cc.end();it++)
    {
        uint32_t spuid = it->first;
        const Product& p = products_[spuid];
        SpuMatchCandidate smc;
        bool cid_found = false;
        for(uint32_t i=0;i<cid_list.size();i++)
        {
            if(EqualOrIsParent_(cid_list[i], p.cid) || EqualOrIsParent_(p.cid, cid_list[i]))
            {
                cid_found = true;
                smc.cid_index = i;
                break;
            }
        }
        if(!cid_found) continue;
        SpuContributorValue& scv = it->second;
        if(scv.brand_match)
        {
            if(p.sbrand.length()>found_brand.length())
            {
                found_brand = p.sbrand;
            }
        }
        if(!price.Positive()) continue;
        bool pricesim = IsPriceSim_(price, p.price);
#ifdef B5M_DEBUG
        LOG(ERROR)<<p.stitle<<",["<<price<<","<<p.price<<"],"<<(int)pricesim<<std::endl;
#endif
        if(!pricesim) continue;
#ifdef B5M_DEBUG
        LOG(ERROR)<<p.stitle<<","<<scv.GetLenWeight()<<","<<text_term_len<<","<<scv.paweight<<","<<p.aweight<<std::endl;
#endif
        //scv.lenweight/=text_term_len;
        if(IsSpuMatch_(p, scv, text_term_len))
        {
            smc.spuid = spuid;
            smc.paweight = scv.paweight;
            smc.lenweight = scv.GetLenWeight();
            smc.price_diff = PriceDiff_(price, p.price);
            spu_match_candidates.push_back(smc);
        }
    }
    if(!spu_match_candidates.empty())
    {
        std::sort(spu_match_candidates.begin(), spu_match_candidates.end());
        const Product& matchp = products_[spu_match_candidates.front().spuid];
        result_products.push_back(matchp);
        result_products.back().score = 1.0;
    }
    else
    {
        std::size_t count = std::min((std::size_t)limit, cid_list.size());
        for(std::size_t i=0;i<count;i++)
        {
            Product p;
            p.scategory = category_list_[cid_list[i]].name;
            p.score = cid_score_list[i];
            if(!found_brand.empty()) p.sbrand = found_brand;
            result_products.push_back(p);
        }
    }

}

void ProductMatcher::Compute_(const Document& doc, const std::vector<Term>& term_list, KeywordVector& keyword_vector, uint32_t limit, std::vector<Product>& result_products)
{
    Document::doc_prop_value_strtype title;
    doc.getProperty("Title", title);
    std::string stitle = propstr_to_str(title);
#ifdef B5M_DEBUG
    std::cout<<"[TITLE]"<<stitle<<std::endl;
#endif
    double price = 0.0;
    Document::doc_prop_value_strtype uprice;
    if(doc.getProperty("Price", uprice))
    {
        ProductPrice pp;
        pp.Parse(propstr_to_ustr(uprice));
        pp.GetMid(price);
    }
    SimObject title_obj;
    string_similarity_.Convert(title, title_obj);
    bool matcher_only = matcher_only_;
    uint32_t given_cid = 0;
    Document::doc_prop_value_strtype given_category;
    doc.getProperty("Category", given_category);
    std::string sgiven_category = propstr_to_str(given_category);
    if(!given_category.empty())
    {
        CategoryIndex::const_iterator it = category_index_.find(sgiven_category);
        if(it!=category_index_.end())
        {
            given_cid = it->second;
        }
    }
    else
    {
        matcher_only = false;
    }
    if(matcher_only && given_cid==0) return;
    bool given_cid_leaf = true;
    if(given_cid>0)
    {
        const Category& c = category_list_[given_cid];
        if(c.is_parent)
        {
            given_cid_leaf = false;
        }
    }
    //std::string soid;
    //doc.getString("DOCID", soid);
    //std::cerr<<soid<<",matcher only:"<<matcher_only<<std::endl;
    //firstly do cid;
    typedef dmap<uint32_t, double> IdToScore;
    typedef dmap<uint32_t, WeightType> IdToWeight;
    //typedef std::vector<double> ScoreList;
    //typedef dmap<uint32_t, ScoreList> IdToScoreList;
    //typedef std::map<uint32_t, ProductCandidate> PidCandidates;
    typedef dmap<uint32_t, std::pair<WeightType, uint32_t> > CidSpu;
    //ScoreList max_share_point(3,0.0);
    //max_share_point[0] = 2.0;
    //max_share_point[1] = 1.0;
    //max_share_point[2] = 1.0;
    //IdToScore cid_candidates(0.0);
    IdToWeight pid_weight;
    IdToScore cid_score(0.0);
    CidSpu cid_spu(std::make_pair(WeightType(), 0));
    //PidCandidates pid_candidates;
    //ScoreList default_score_list(3, 0.0);
    //IdToScoreList cid_share_list(default_score_list);
    //uint32_t keyword_count = keyword_vector.size();
    typedef boost::unordered_set<std::pair<uint32_t, std::string> > SpuAttributeApp;
    SpuAttributeApp sa_app;
    std::size_t text_term_len = 0;
    for(uint32_t i=0;i<term_list.size();i++)
    {
        if(!IsSymbol_(term_list[i]))
        {
            ++text_term_len;
        }
    }
    for(KeywordVector::const_iterator it = keyword_vector.begin();it!=keyword_vector.end();++it)
    {
        const TermList& tl = it->term_list;
        const KeywordTag& tag = *it;
        double kweight = tag.kweight;
        //double aweight = tag.aweight;
        double terms_length = tl.size()-(tag.ngram-1);
        double len_weight = terms_length/text_term_len;
        //double count_weight = 1.0/keyword_count;
#ifdef B5M_DEBUG
        std::string text = GetText_(tl);
        std::cout<<"[KEYWORD]"<<text<<","<<kweight<<","<<std::endl;
#endif

        for(uint32_t i=0;i<tag.category_name_apps.size();i++)
        {
            const CategoryNameApp& app = tag.category_name_apps[i];
            if(matcher_only)
            {
                if(!EqualOrIsParent_(given_cid, app.cid)) continue;
            }
            double depth_ratio = 1.0-0.2*(app.depth-1);
            double times = 1.0;
            //if(count_weight>=0.8) times = 2.5;
            //else if(count_weight>=0.5) times = 2.0;
            double share_point = times*depth_ratio;
            if(!app.is_complete) share_point*=0.6;
            double add_score = share_point*kweight;
            cid_score[app.cid] += add_score;
#ifdef B5M_DEBUG
            //std::cerr<<"[CNA]"<<category_list_[app.cid].name<<","<<app.depth<<","<<app.is_complete<<","<<add_score<<std::endl;
#endif
        }
        for(uint32_t i=0;i<tag.offer_category_apps.size();i++)
        {
            const OfferCategoryApp& app = tag.offer_category_apps[i];
            if(matcher_only)
            {
                if(!EqualOrIsParent_(given_cid, app.cid)) continue;
            }
            uint32_t app_count = app.count;
            if(app_count>20) app_count = 20;
            double depth_ratio = 1.0;
            double times = app_count*0.04;
            //if(count_weight>=0.8) times *= 2.5;
            //else if(count_weight>=0.5) times *= 2.0;
            double share_point = times*depth_ratio;
            double add_score = share_point*kweight;
            cid_score[app.cid] += add_score;
#ifdef B5M_DEBUG
            //std::cerr<<"[CNO]"<<category_list_[app.cid].name<<","<<add_score<<std::endl;
#endif
        }
        for(uint32_t i=0;i<tag.attribute_apps.size();i++)
        {
            const AttributeApp& app = tag.attribute_apps[i];
            if(app.spu_id==0) continue;
            //if(app.is_optional) continue;
            const Product& p = products_[app.spu_id];
            if(matcher_only)
            {
                if(!EqualOrIsParent_(given_cid, p.cid)) continue;
            }
            double psim = PriceSim_(price, p.price.Mid());
            if(psim<0.25) continue;
            std::pair<uint32_t, std::string> sa_app_value(app.spu_id, app.attribute_name);
            if(sa_app.find(sa_app_value)!=sa_app.end()) continue;
            double share_point = 0.0;
            double p_point = 0.0;
            if(app.is_optional)
            {
                share_point = 0.1;
                p_point = 0.1;
            }
            else if(app.attribute_name=="型号")
            {
                share_point = 0.3;
                p_point = 1.5;
            }
            else
            {
                share_point = 0.2;
                p_point = 1.0;
            }
            WeightType& wt = pid_weight[app.spu_id];
            wt.aweight+=share_point*kweight;
            double paweight = p_point;
            wt.paweight+=paweight;
            wt.paratio+=len_weight;
            if(app.attribute_name=="型号") wt.type_match = true;
            if(app.attribute_name=="品牌") wt.brand_match = true;
            //double pprice = 0.0;
            //p.price.GetMid(pprice);
            wt.price_diff = PriceDiff_(price, p.price.Mid());
            sa_app.insert(sa_app_value);
#ifdef B5M_DEBUG
            //std::cerr<<"[AN]"<<category_list_[p.cid].name<<","<<share_point*kweight<<std::endl;
#endif
        }
        //for(uint32_t i=0;i<tag.spu_title_apps.size();i++)
        //{
            //const SpuTitleApp& app = tag.spu_title_apps[i];
            //if(app.spu_id==0) continue;
//#ifdef B5M_DEBUG
            ////std::cout<<"[TS]"<<products_[app.spu_id].stitle<<std::endl;
//#endif
            //const Product& p = products_[app.spu_id];
            //if(matcher_only)
            //{
                //if(!EqualOrIsParent_(given_cid, p.cid)) continue;
            //}
            //double psim = PriceSim_(price, p.price);
            //pid_weight[app.spu_id].tweight+=0.2*weight*psim;
        //}

    }
    //return;
    for(IdToWeight::const_iterator it = pid_weight.begin();it!=pid_weight.end();it++)
    {
        uint32_t spu_id = it->first;
        const Product& p = products_[spu_id];
        const WeightType& weight = it->second;
        uint32_t cid = p.cid;
        std::pair<WeightType, uint32_t>& e_weight_spuid = cid_spu[cid];
        WeightType& eweight = e_weight_spuid.first;
        uint32_t e_spu_id = e_weight_spuid.second;
        const Product& ep = products_[e_spu_id];
        bool ematched = SpuMatched_(eweight, ep);

        bool matched = SpuMatched_(weight, p);
#ifdef B5M_DEBUG
        std::cerr<<category_list_[cid].name<<","<<p.stitle<<","<<ematched<<","<<matched<<","<<eweight.sum()<<","<<weight.sum()<<std::endl;
#endif
        if(ematched&&matched)
        {
            bool replace = false;
            if(weight.paweight>eweight.paweight) replace = true;
            else if(weight.paweight==eweight.paweight)
            {
                if(eweight.price_diff!=weight.price_diff)
                {
                    if(weight.price_diff<eweight.price_diff) replace = true;
                }
                else
                {
                    if(weight.sum()>eweight.sum())
                    {
                        replace = true;
                    }
                }
            }
            if(replace)
            {
                e_weight_spuid.first = weight;
                e_weight_spuid.second = spu_id;
            }
        }
        else if(!ematched&&matched)
        {
            if(weight.sum()>eweight.sum()*0.6)
            {
                e_weight_spuid.first = weight;
                e_weight_spuid.second = spu_id;
            }
        }
        else if(!ematched&&!matched)
        {
            if(weight.sum()>=eweight.sum())
            {
                e_weight_spuid.second = spu_id;
                e_weight_spuid.first = weight;
            }
        }
    }
    for(CidSpu::iterator it = cid_spu.begin();it!=cid_spu.end();it++)
    {
        uint32_t cid = it->first;
        std::pair<WeightType, uint32_t>& e = it->second;
        IdToScore::const_iterator sit = cid_score.find(cid);
        if(sit!=cid_score.end())
        {
            e.first.cweight+=sit->second;
            cid_score.erase(sit);
        }

    }
    for(IdToScore::const_iterator it = cid_score.begin();it!=cid_score.end();it++)
    {
        uint32_t cid = it->first;
        double score = it->second;
        const IdList& pid_list = cid_to_pids_[cid];
        if(!pid_list.empty())
        {
            uint32_t spu_id = pid_list.front();
            std::pair<WeightType, uint32_t>& e = cid_spu[cid];
            e.first.cweight+=score;
            e.second = spu_id;
        }
    }
    std::vector<ResultVectorItem> result_vector;
    result_vector.reserve(cid_spu.size());
    double max_score = 0.0;
    for(CidSpu::iterator it = cid_spu.begin();it!=cid_spu.end();++it)
    {
        uint32_t cid = it->first;
        uint32_t spu_id = it->second.second;
        //const Product& p = products_[spu_id];
        //const Category& c = category_list_[cid];
        WeightType& weight = it->second.first;
        //post-process on weight

        //weight.tweight*=string_similarity_.Sim(title_obj, p.title_obj);
        //if(weight.tweight>0.0&&p.tweight>0.0)
        //{
            //weight.tweight/=p.tweight;
        //}
        //if(weight.cweight>0.0)
        //{
            //weight.cweight*=std::log(10.0-c.depth);
        //}
        //else
        //{
            ////weight.kweight*=std::log(8.0+keyword_count-c.depth);
        //}
        //weight.kweight*=std::log(28.0+keyword_count-c.depth);
        ResultVectorItem item;
        item.cid = cid;
        item.spu_id = spu_id;
        item.score = weight.sum();
        item.weight = weight;
        item.is_given_category = false;
        if(item.score>max_score) max_score = item.score;
        result_vector.push_back(item);
#ifdef B5M_DEBUG
        std::cerr<<"[CSC]"<<category_list_[cid].name<<","<<weight<<std::endl;
#endif
    }
    for(uint32_t i=0;i<result_vector.size();i++)
    {
        if(result_vector[i].cid==given_cid&&given_cid_leaf)
        {
            result_vector[i].score = max_score;
            result_vector[i].is_given_category = true;
        }
    }
    if(result_vector.empty()) return;
    std::sort(result_vector.begin(), result_vector.end());
    const Product& max_p = products_[result_vector.front().spu_id];
    const Category& max_c = category_list_[max_p.cid];
    static const uint32_t MAX_SPU_CANDIDATE = 3;

    double score_limit = 0.9;
    if(limit>1)
    {
        score_limit = 0.7;
    }
    score_limit = max_score*score_limit;
    std::vector<Product> result_candidates;
    for(uint32_t i=0;i<result_vector.size();i++)
    {
        uint32_t spu_id = result_vector[i].spu_id;
        double score = result_vector[i].score;
        const Product& p = products_[spu_id];
        const Category& c = category_list_[p.cid];
        //std::cerr<<"[CNAME]"<<c.name<<","<<max_c.name<<std::endl;
        if(!boost::algorithm::starts_with(c.name, max_c.name))
        {
            if(score<0.4) break;
            if(score<score_limit) break;
        }
        const WeightType& weight = result_vector[i].weight;
#ifdef B5M_DEBUG
        uint32_t cid = result_vector[i].cid;
        std::cerr<<"[CC]"<<category_list_[cid].name<<","<<products_[spu_id].stitle<<","<<score<<std::endl;
#endif
        if(i<MAX_SPU_CANDIDATE)
        {
            if(SpuMatched_(weight, p))
            {
#ifdef B5M_DEBUG
                std::cerr<<"[MATCHED]"<<p.stitle<<std::endl;
#endif
                //if(category_max_depth_>0)
                //{
                    //matched_p.cid = GetCidByMaxDepth_(matched_p.cid);
                    //if(matched_p.cid==0) continue;
                    //matched_p.scategory = category_list_[matched_p.cid].name;
                //}
                result_products.resize(1);
                result_products[0] = p;
                result_products[0].score = score;
                return;
            }
        }
        Product cp;
        cp.score = score;
        cp.scategory = p.scategory;
        if(weight.brand_match)
        {
            UString brand(p.sbrand, UString::UTF_8);
            if(brand.length()>=7)
            {
                cp.sbrand = p.sbrand;
            }
        }
        result_candidates.push_back(cp);
        //result_products.push_back(cp);
        //if(result_products.size()>=limit) break;
    }
    uint32_t count = std::min((uint32_t)result_candidates.size(), limit);
    result_products.assign(result_candidates.begin(), result_candidates.begin()+count);
    //result_vector.resize(i);
    //if(result_vector.empty()) return;
    //if(!match_found)
    //{
        //boost::unordered_set<uint32_t> cid_set;
        //for(uint32_t r=0;r<result_vector.size();r++)
        //{
            //if(result_products.size()>=limit) break;
            //Product result_product;
            //const ResultVectorItem& item = result_vector[r];
            //uint32_t spu_id = item.spu_id;
            //const Product& p = products_[spu_id];
            //uint32_t cid = item.cid;
            //if(category_max_depth_>0)
            //{
                //cid = GetCidByMaxDepth_(cid);
                //if(cid==0) continue;
                //if(cid_set.find(cid)!=cid_set.end()) continue;
                //cid_set.insert(cid);
            //}
            //const WeightType& weight = item.weight;
            //if(weight.brand_match)
            //{
                //UString brand(p.sbrand, UString::UTF_8);
                //if(brand.length()>=7)
                //{
                    //result_product.sbrand = p.sbrand;
                //}
            //}
            //result_product.scategory = category_list_[cid].name;
            //result_products.push_back(result_product);
        //}
    //}
}

double ProductMatcher::PriceSim_(double offerp, double spup) const
{
    if(!use_price_sim_) return 0.25;
    if(spup==0.0) return 0.25;
    if(offerp==0.0) return 0.0;
    if(offerp>spup) return spup/offerp;
    else return offerp/spup;
}

bool ProductMatcher::IsValuePriceSim_(double op, double p) const
{
    if(p<=0.0) return true;
    if(op<=0.0) return false;
    static const double thlow = 0.25;
    double thhigh = 4.0;
    if(p<=100.0) thhigh = 4.0;
    else if(p<=5000.0) thhigh = 3.0;
    else thhigh = 2.0;
    double v = op/p;
    if(v>=thlow&&v<=thhigh) return true;
    else return false;
}

bool ProductMatcher::IsPriceSim_(const ProductPrice& op, const ProductPrice& p) const
{
    if(!p.Positive()) return true;
    if(!op.Positive()) return false;
    if(IsValuePriceSim_(op.Min(), p.Min())) return true;
    if(IsValuePriceSim_(op.Max(), p.Max())) return true;
    return false;
}

double ProductMatcher::PriceDiff_(double op, double p) const
{
    if(op<=0.0||p<=0.0) return 999999.00;
    return std::abs(op-p);
}
double ProductMatcher::PriceDiff_(const ProductPrice& op, const ProductPrice& p) const
{
    if(!p.Positive()||!op.Positive()) return 999999.00;
    return std::min( std::abs(p.Min()-op.Min()), std::abs(p.Max()-op.Max()));
}

void ProductMatcher::Analyze_(const izenelib::util::UString& btext, std::vector<Term>& result)
{
    izenelib::util::UString text(btext);
    text.toLowerString();
    std::string stext;
    text.convertString(stext, UString::UTF_8);
    std::vector<idmlib::util::IDMTerm> term_list;
    chars_analyzer_->GetTermList(text, term_list);
    result.reserve(term_list.size());
    std::size_t spos = 0;
    bool last_match = true;
    for(std::size_t i=0;i<term_list.size();i++)
    {
        std::string str;
        term_list[i].text.convertString(str, izenelib::util::UString::UTF_8);
        term_t id = GetTerm_(str);
        //std::cerr<<"[A]"<<str<<","<<term_list[i].tag<<","<<term_list[i].position<<","<<spos<<","<<last_match<<std::endl;
        std::size_t pos = stext.find(str, spos);
        bool has_blank = false;
        if(pos!=std::string::npos)
        {
            if(pos>spos && last_match)
            {
                has_blank = true;
            }
            spos = pos+str.length();
            last_match = true;
        }
        else
        {
            last_match = false;
#ifdef B5M_DEBUG
            //std::cerr<<"[TE]"<<str<<std::endl;
            //std::cerr<<"[TITLE]"<<stext<<std::endl;
#endif
        }
        if(has_blank)
        {
            std::string append = blank_;
            term_t app_id = GetTerm_(append);
            Term term;
            term.id = app_id;
            term.text = UString(append, UString::UTF_8);
            term.tag = idmlib::util::IDMTermTag::SYMBOL;
            if(stext.at(pos-1) != ' ')term.position = -1;
            result.push_back(term);
        }

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
            //else
            //{
                //if(!IsTextSymbol_(id))
                //{
                    //symbol_terms_.insert(id);
                //}
            //}
        }
        id = GetTerm_(str);
        term_list[i].id = id;
        result.push_back(term_list[i]);
    }
}

void ProductMatcher::AnalyzeNoSymbol_(const izenelib::util::UString& btext, std::vector<Term>& result)
{
    izenelib::util::UString text(btext);
    text.toLowerString();
    //std::string stext;
    //text.convertString(stext, UString::UTF_8);
    //std::cerr<<"[ANSS]"<<stext<<std::endl;
    std::vector<idmlib::util::IDMTerm> term_list;
    chars_analyzer_->GetTermList(text, term_list);
    result.reserve(term_list.size());
    for(std::size_t i=0;i<term_list.size();i++)
    {
        std::string str;
        term_list[i].text.convertString(str, izenelib::util::UString::UTF_8);
        term_list[i].id = GetTerm_(str);
        //std::cerr<<"[ANS]"<<str<<","<<term_list[i].tag<<std::endl;
        char tag = term_list[i].tag;
        //std::cerr<<"ARB,"<<str<<std::endl;
        if(tag == idmlib::util::IDMTermTag::SYMBOL)
        {
            if(!IsTextSymbol_(term_list[i].id))
            {
                continue;
            }
        }
        result.push_back(term_list[i]);

    }
}


void ProductMatcher::ParseAttributes(const UString& ustr, std::vector<Attribute>& attributes)
{
    std::vector<AttrPair> attrib_list;
    std::vector<std::pair<UString, std::vector<UString> > > my_attrib_list;
    split_attr_pair(ustr, attrib_list);
    for(std::size_t i=0;i<attrib_list.size();i++)
    {
        Attribute attribute;
        attribute.is_optional = false;
        const std::vector<izenelib::util::UString>& attrib_value_list = attrib_list[i].second;
        if(attrib_value_list.empty()) continue;
        izenelib::util::UString attrib_value = attrib_value_list[0];
        for(uint32_t a=1;a<attrib_value_list.size();a++)
        {
            attrib_value.append(UString(" ",UString::UTF_8));
            attrib_value.append(attrib_value_list[a]);
        }
        if(attrib_value.empty()) continue;
        //if(attrib_value_list.size()!=1) continue; //ignore empty value attrib and multi value attribs
        izenelib::util::UString attrib_name = attrib_list[i].first;
        //if(attrib_value.length()==0 || attrib_value.length()>30) continue;
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
        //if(attribute.name=="容量" && attribute.values.size()==1 && boost::algorithm::ends_with(attribute.values[0], "G"))
        //{
            //std::string gb_value = attribute.values[0]+"B";
            //attribute.values.push_back(gb_value);
        //}
        attributes.push_back(attribute);
    }
}

void ProductMatcher::MergeAttributes(std::vector<Attribute>& eattributes, const std::vector<Attribute>& attributes)
{
    if(attributes.empty()) return;
    boost::unordered_set<std::string> to_append_name;
    for(uint32_t i=0;i<attributes.size();i++)
    {
        to_append_name.insert(attributes[i].name);
    }
    for(uint32_t i=0;i<eattributes.size();i++)
    {
        to_append_name.erase(eattributes[i].name);
    }
    for(uint32_t i=0;i<attributes.size();i++)
    {
        const ProductMatcher::Attribute& a = attributes[i];
        if(to_append_name.find(a.name)!=to_append_name.end())
        {
            eattributes.push_back(a);
        }
    }
}

UString ProductMatcher::AttributesText(const std::vector<Attribute>& attributes)
{
    std::string str;
    for(uint32_t i=0;i<attributes.size();i++)
    {
        const ProductMatcher::Attribute& a = attributes[i];
        if(!str.empty()) str+=",";
        str+=a.GetText();
    }
    return UString(str, UString::UTF_8);
}

ProductMatcher::term_t ProductMatcher::GetTerm_(const UString& text)
{
    term_t term = izenelib::util::HashFunction<izenelib::util::UString>::generateHash32(text);
    //term_t term = izenelib::util::HashFunction<izenelib::util::UString>::generateHash64(text);
#ifdef B5M_DEBUG
    id_manager_[term] = text;
#endif
    return term;
}
ProductMatcher::term_t ProductMatcher::GetTerm_(const std::string& text)
{
    UString utext(text, UString::UTF_8);
    return GetTerm_(utext);
}

std::string ProductMatcher::GetText_(const TermList& tl, const std::string& s) const
{
    std::string result;
#ifdef B5M_DEBUG
    for(uint32_t i=0;i<tl.size();i++)
    {
        std::string str;
        term_t term = tl[i];
        IdManager::const_iterator it = id_manager_.find(term);
        if(it!=id_manager_.end())
        {
            const UString& ustr = it->second;
            ustr.convertString(str, UString::UTF_8);
        }
        else
        {
            str = "__UNKNOW__";
        }
        if(i>0) result+=s;
        result+=str;
    }
#endif
    return result;
}
std::string ProductMatcher::GetText_(const term_t& term) const
{
#ifdef B5M_DEBUG
    IdManager::const_iterator it = id_manager_.find(term);
    if(it!=id_manager_.end())
    {
        const UString& ustr = it->second;
        std::string str;
        ustr.convertString(str, UString::UTF_8);
        return str;
    }
#endif
    return "";
}

void ProductMatcher::GetTerms_(const UString& text, std::vector<term_t>& term_list)
{
    //TODO: this will return one token 'd90' for "D90" input in sf1r enviorment, why?
    std::vector<Term> t_list;
    AnalyzeNoSymbol_(text, t_list);
    term_list.resize(t_list.size());
    for(uint32_t i=0;i<t_list.size();i++)
    {
        term_list[i] = t_list[i].id;
    }
}
//void ProductMatcher::GetCRTerms_(const UString& text, std::vector<term_t>& term_list)
//{
    //std::vector<UString> text_list;
    //AnalyzeCR_(text, text_list);
    //term_list.resize(text_list.size());
    //for(uint32_t i=0;i<term_list.size();i++)
    //{
        //term_list[i] = GetTerm_(text_list[i]);
    //}
//}
void ProductMatcher::GetTerms_(const std::string& text, std::vector<term_t>& term_list)
{
    UString utext(text, UString::UTF_8);
    GetTerms_(utext, term_list);
}
void ProductMatcher::GetTermsString_(const UString& text, std::string& str)
{
    std::vector<Term> t_list;
    AnalyzeNoSymbol_(text, t_list);
    for(uint32_t i=0;i<t_list.size();i++)
    {
        std::string s;
        t_list[i].text.convertString(s, UString::UTF_8);
        str+=s;
    }
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
    for(uint32_t i=1;i<category_list_.size();i++)
    {
        if(i%100==0)
        {
            LOG(INFO)<<"scanning category "<<i<<std::endl;
            //LOG(INFO)<<"max tid "<<aid_manager_->getMaxDocId()<<std::endl;
        }
        Category category = category_list_[i];
        if(category.name.empty()) continue;
        uint32_t cid = category.cid;
        const std::vector<std::string>& keywords = category.keywords;
        for(uint32_t k=0;k<keywords.size();k++)
        {
            const std::string& w = keywords[k];
            //if(app.find(w)!=app.end()) continue;
            std::vector<term_t> term_list;
            GetTerms_(w, term_list);
            CategoryNameApp cn_app;
            cn_app.cid = cid;
            cn_app.depth = 1;
            cn_app.is_complete = true;
            trie[term_list].category_name_apps.push_back(cn_app);
            //trie[term_list].text = UString(w, UString::UTF_8);
            //Suffixes suffixes;
            //GenSuffixes_(term_list, suffixes);
            //for(uint32_t s=0;s<suffixes.size();s++)
            //{
                //CategoryNameApp cn_app;
                //cn_app.cid = cid;
                //cn_app.depth = depth;
                //cn_app.is_complete = false;
                //if(s==0) cn_app.is_complete = true;
                //trie[suffixes[s]].category_name_apps.push_back(cn_app);
            //}
            //app.insert(w);
        }
        //uint32_t depth = 1;
        //uint32_t ccid = cid;
        //Category ccategory = category;
        //std::set<std::string> app;
        //while(ccid>0)
        //{
            //const std::vector<std::string>& keywords = ccategory.keywords;
            //for(uint32_t k=0;k<keywords.size();k++)
            //{
                //const std::string& w = keywords[k];
                //if(app.find(w)!=app.end()) continue;
                //std::vector<term_t> term_list;
                //GetTerms_(w, term_list);
                //Suffixes suffixes;
                //GenSuffixes_(term_list, suffixes);
                //for(uint32_t s=0;s<suffixes.size();s++)
                //{
                    //CategoryNameApp cn_app;
                    //cn_app.cid = cid;
                    //cn_app.depth = depth;
                    //cn_app.is_complete = false;
                    //if(s==0) cn_app.is_complete = true;
                    //trie[suffixes[s]].category_name_apps.push_back(cn_app);
                //}
                //app.insert(w);
            //}
            //ccid = ccategory.parent_cid;
            //ccategory = category_list_[ccid];
            //depth++;
        //}
    }
    for(uint32_t i=1;i<products_.size();i++)
    {
        if(i%100000==0)
        {
            LOG(INFO)<<"scanning product "<<i<<std::endl;
            //LOG(INFO)<<"max tid "<<aid_manager_->getMaxDocId()<<std::endl;
        }
        uint32_t pid = i;
        const Product& product = products_[i];
        //std::vector<term_t> title_terms;
        //GetTerms_(product.stitle, title_terms);
        //Suffixes title_suffixes;
        //GenSuffixes_(title_terms, title_suffixes);
        //for(uint32_t s=0;s<title_suffixes.size();s++)
        //{
            //SpuTitleApp st_app;
            //st_app.spu_id = pid;
            //st_app.pstart = s;
            //st_app.pend = title_terms.size();
            //trie[title_suffixes[s]].spu_title_apps.push_back(st_app);
        //}
        const std::vector<Attribute>& attributes = product.attributes;
        for(uint32_t a=0;a<attributes.size();a++)
        {
            const Attribute& attribute = attributes[a];
            for(uint32_t v=0;v<attribute.values.size();v++)
            {
                AttributeApp a_app;
                a_app.spu_id = pid;
                a_app.attribute_name = attribute.name;
                a_app.is_optional = attribute.is_optional;
                std::vector<term_t> terms;
                GetTerms_(attribute.values[v], terms);
                trie[terms].attribute_apps.push_back(a_app);
                //trie[terms].text = UString(attribute.values[v], UString::UTF_8);
                //if(!attribute.is_optional&&NeedFuzzy_(attribute.values[v]))
                //{
//#ifdef B5M_DEBUG
                    //std::cerr<<"need fuzzy "<<attribute.values[v]<<std::endl;
//#endif
                    //Suffixes suffixes;
                    //GenSuffixes_(attribute.values[v], suffixes);
                    //for(uint32_t s=0;s<suffixes.size();s++)
                    //{
                        //FuzzyApp app;
                        //app.spu_id = pid;
                        //app.attribute_name = attribute.name;
                        //app.term_list = terms;
                        //app.pos.begin = s;
                        //ftrie_[suffixes[s]].fuzzy_apps.push_back(app);
                    //}
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
    boost::algorithm::to_lower(str);
    //std::cerr<<"[AK]"<<str<<std::endl;
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
    if(value_type==2 && text.length()<2) return;
    if(value_type==1 && text.length()<3) return;
    std::vector<Term> term_list;
    //std::vector<term_t> term_list;
    AnalyzeNoSymbol_(text, term_list);
    if(term_list.empty()) return;
    std::vector<term_t> id_list(term_list.size());
    for(uint32_t i=0;i<term_list.size();i++)
    {
        id_list[i] = term_list[i].id;
    }
    if(not_keywords_.find(id_list)!=not_keywords_.end()) return;
    if(term_list.size()==1)
    {
        if(term_list[0].text.length()<2) return;
    }
    keyword_set_.insert(id_list);
    keyword_text_[id_list] = text;
    //std::cout<<"[AKT]"<<GetText_(term_list);
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
        //if(category.is_parent) continue;
        if(category.name.empty()) continue;
        std::vector<std::string> cs_list;
        boost::algorithm::split( cs_list, category.name, boost::algorithm::is_any_of(">") );

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
        for(uint32_t k=0;k<category.keywords.size();k++)
        {
            AddKeyword_(UString(category.keywords[k], UString::UTF_8));
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
        //std::cerr<<product.spid<<std::endl;
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
    all_keywords_.resize(keywords.size()+1);
    for(uint32_t k=0;k<keywords.size();k++)
    {
        if(k%100000==0)
        {
            LOG(INFO)<<"build keyword "<<k<<std::endl;
        }
        uint32_t keyword_id = k+1;
        const TermList& keyword = keywords[k];
        //std::string keyword_str = GetText_(keyword);
        //LOG(INFO)<<"keyword "<<keyword_str<<std::endl;
        KeywordTag tag;
        tag.id = keyword_id;
        tag.term_list = keyword;
        //find keyword in suffix_trie
        //for(TrieType::const_iterator it = suffix_trie.lower_bound(keyword);it!=suffix_trie.end();it++)
        //{
            //const TermList& key = it->first;
            ////std::string key_str = GetText_(key);
            //const KeywordTag& value = it->second;
            //if(StartsWith_(key, keyword))
            //{
                //bool is_complete = false;
                //if(key==keyword) is_complete = true;
                ////LOG(INFO)<<"key found "<<key_str<<std::endl;
                //tag.Append(value, is_complete);
                ////tag+=value;
            //}
            //else
            //{
                ////LOG(INFO)<<"key break "<<key_str<<std::endl;
                //break;
            //}
        //}
        TrieType::const_iterator it = suffix_trie.find(keyword);
        if(it!=suffix_trie.end())
        {
            const KeywordTag& value = it->second;
            tag.Append(value, true);//is complete
        }
        tag.Flush();
        //for(uint32_t i=0;i<tag.category_name_apps.size();i++)
        //{
            //const CategoryNameApp& app = tag.category_name_apps[i];
            //if(app.is_complete)
            //{
                //tag.type_app["c|"+boost::lexical_cast<std::string>(app.cid)] = 1;
                ////break;
            //}
        //}
        //if(tag.type_app.empty())
        //{
            //for(uint32_t i=0;i<tag.attribute_apps.size();i++)
            //{
                //const AttributeApp& app = tag.attribute_apps[i];
                //if(app.spu_id==0) continue;
                ////if(app.is_optional) continue;
                //const Product& p = products_[app.spu_id];
                //uint32_t cid = p.cid;
                //std::string key = boost::lexical_cast<std::string>(cid)+"|"+app.attribute_name;
                //tag.type_app[key]+=1;
                ////double share_point = 0.0;
                ////if(app.is_optional) share_point = 0.5;
                ////else if(app.attribute_name=="型号") share_point = 1.5;
                ////else share_point = 1.0;
                ////sts["a|"+app.attribute_name]+=share_point;
            //}
        //}
        tag.text = keyword_text_[keyword];
        //std::string stext;
        //tag.text.convertString(stext, UString::UTF_8);
        //std::cerr<<"tag:"<<stext<<std::endl;
        trie_[keyword] = tag;
        all_keywords_[keyword_id] = tag;
    }
    //post-process
#ifdef B5M_DEBUG
    for(TrieType::iterator it = trie_.begin();it!=trie_.end();it++)
    {
        KeywordTag& tag = it->second;
        UString utext = keyword_text_[tag.term_list];
        std::string text;
        utext.convertString(text, UString::UTF_8);
        if(!tag.category_name_apps.empty())
        {
            std::cerr<<"XXKTC,"<<text<<std::endl;
        }
        bool has_brand = false;
        bool has_type = false;
        bool has_other = false;
        for(uint32_t i=0;i<tag.attribute_apps.size();i++)
        {
            const AttributeApp& app = tag.attribute_apps[i];
            if(app.spu_id==0) continue;
            if(app.attribute_name=="品牌")
            {
                has_brand = true;
            }
            else if(app.attribute_name=="型号")
            {
                has_type = true;
            }
            else
            {
                has_other = true;
            }
        }
        if(has_brand)
        {
            std::cerr<<"XXKTB,"<<text<<std::endl;
        }
        if(has_type)
        {
            std::cerr<<"XXKTT,"<<text<<std::endl;
        }
        if(has_other)
        {
            std::cerr<<"XXKTO,"<<text<<std::endl;
        }
    }
#endif
}
bool ProductMatcher::SpuMatched_(const WeightType& weight, const Product& p) const
{
    if(p.spid.empty()) return false; //detect empty product
    double paweight = weight.paweight;
    if(weight.paratio>=0.8&&weight.type_match) paweight*=2;
    if(paweight>=1.5&&paweight>=p.aweight) return true;
    return false;
}
bool ProductMatcher::IsSpuMatch_(const Product& p, const SpuContributorValue& scv, double query_len) const
{
    if(p.spid.empty()) return false; //detect empty product
    double paweight = scv.paweight;
    //LOG(INFO)<<p.stitle<<","<<scv.lenweight<<","<<scv.model_match<<","<<paweight<<std::endl;
    double ratio_lenweight = scv.GetLenWeight()/query_len;
    if(ratio_lenweight>=0.8&&scv.model_match) paweight*=2;
    if(paweight>=1.5&&paweight>=p.aweight) return true;
    return false;
}

int ProductMatcher::SelectKeyword_(const KeywordTag& tag1, const KeywordTag& tag2) const
{
    //if(tag1.term_list==tag2.term_list)
    //{
        //if(tag1.kweight>tag2.kweight)
        //{
            //return 1;
        //}
        //return 2;
    //}
    //const Position& position1 = tag1.positions.front();
    //const Position& position2 = tag2.positions.front();
    //if( (position1.first>=position2.first&&position1.second<=position2.second)
      //|| (position2.first>=position1.first&&position2.second<=position1.second))
    //{
        ////overlapped
        //const KeywordTag* container = NULL;
        //const KeywordTag* be_contained = NULL;
        //int pcontainer = 1;

        //if( (position1.first>=position2.first&&position1.second<=position2.second) )
        //{
            //container = &tag2;
            //pcontainer = 2;
            //be_contained = &tag1;
        //}
        //else
        //{
            //container = &tag1;
            //pcontainer = 1;
            //be_contained = &tag2;
        //}
        //bool has_same = false;
        //for(KeywordTypeApp::const_iterator tit = be_contained->type_app.begin();tit!=be_contained->type_app.end();tit++)
        //{
            //const std::string& key = tit->first;
            //if(container->type_app.find(key)!=container->type_app.end())
            //{
                //has_same = true;
                //break;
            //}
        //}
        //if(has_same) return pcontainer;
        //return 3;
    //}
    return 3;
}
bool ProductMatcher::IsBlankSplit_(const UString& t1, const UString& t2) const
{
    if(t1.length()==0||t2.length()==0) return true;
    if(t1.isDigitChar(t1.length()-1) && t2.isDigitChar(0)) return true;
    return false;
}
ProductMatcher::cid_t ProductMatcher::GetLevelCid_(const std::string& scategory, uint32_t level) const
{
    if(level==0) return 0;
    std::vector<std::string> vec;
    boost::algorithm::split(vec, scategory, boost::algorithm::is_any_of(">"));
    if(vec.size()<level) return 0;
    std::string category;
    for(uint16_t i=0;i<level;i++)
    {
        if(!category.empty()) category+=">";
        category+=vec[i];
    }
    CategoryIndex::const_iterator cit = category_index_.find(category);;
    if(cit==category_index_.end()) return 0;
    return cit->second;
}

void ProductMatcher::GenFeatureVector_(const std::vector<KeywordTag>& keywords, FeatureVector& feature_vector) const
{
    for(uint32_t i=0;i<keywords.size();i++)
    {
        const KeywordTag& tag = keywords[i];
        double weight = tag.kweight;
        if(weight>0.0)
        {
            uint32_t id = tag.id;
            feature_vector.push_back(std::make_pair(id, weight));
        }
    }
    std::sort(feature_vector.begin(), feature_vector.end());
    FeatureVectorNorm_(feature_vector);
}
void ProductMatcher::FeatureVectorAdd_(FeatureVector& o, const FeatureVector& a) const
{
    FeatureVector n;
    uint32_t i=0,j=0;
    while(i<o.size()&&j<a.size())
    {
        if(o[i].first<a[j].first)
        {
            n.push_back(o[i++]);
        }
        else if (o[i].first==a[j].first)
        {
            n.push_back(std::make_pair(o[i].first, o[i].second+a[j].second));
            i++;
            j++;
        }
        else
        {
            n.push_back(a[j++]);
        }
    }
    while(i<o.size())
    {
        n.push_back(o[i++]);
    }
    while(j<a.size())
    {
        n.push_back(a[j++]);
    }
    o.swap(n);
}
void ProductMatcher::FeatureVectorNorm_(FeatureVector& v) const
{
    double sum = 0.0;
    for(uint32_t i=0;i<v.size();i++)
    {
        sum += v[i].second*v[i].second;
    }
    sum = std::sqrt(sum);
    for(uint32_t i=0;i<v.size();i++)
    {
        v[i].second /= sum;
    }

}

double ProductMatcher::Cosine_(const FeatureVector& v1, const FeatureVector& v2) const
{
    double c = 0.0;
    uint32_t i=0,j=0;
    while(i<v1.size()&&j<v2.size())
    {
        if(v1[i].first<v2[j].first)
        {
            i++;
        }
        else if (v1[i].first==v2[j].first)
        {
            c += v1[i].second*v2[j].second;
            i++;
            j++;
        }
        else
        {
            j++;
        }
    }
    return c;
}

