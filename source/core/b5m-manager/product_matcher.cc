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
#include <ext/pb_ds/assoc_container.hpp>
#include <ext/pb_ds/trie_policy.hpp>
#include <ext/pb_ds/tag_and_trait.hpp>
using namespace sf1r;
using namespace idmlib::sim;
using namespace idmlib::kpe;
using namespace idmlib::util;

#define Malloc(type,n) (type *)malloc((n)*sizeof(type))

//#define B5M_DEBUG

ProductMatcher::ProductMatcher(const std::string& path)
:path_(path), is_open_(false), aid_manager_(NULL), analyzer_(NULL), char_analyzer_(NULL), chars_analyzer_(NULL),
 test_docid_("7bc999f5d10830d0c59487bd48a73cae"),
 left_bracket_("("), right_bracket_(")"), cr_result_(NULL)
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
    if(cr_result_!=NULL)
    {
        cr_result_->flush();
        delete cr_result_;
    }
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
        std::string logger_file = path_+"/logger";
        logger_.open(logger_file.c_str(), std::ios::out | std::ios::app );
        std::string aid_dir = path_+"/attrib_id";
        boost::filesystem::create_directories(aid_dir);
        aid_manager_ = new AttributeIdManager(aid_dir+"/id");
        idmlib::util::IDMAnalyzerConfig aconfig = idmlib::util::IDMAnalyzerConfig::GetCommonConfig("",cma_path_, "");
        aconfig.symbol = true;
        analyzer_ = new idmlib::util::IDMAnalyzer(aconfig);
        idmlib::util::IDMAnalyzerConfig cconfig = idmlib::util::IDMAnalyzerConfig::GetCommonConfig("","", "");
        //cconfig.symbol = true;
        char_analyzer_ = new idmlib::util::IDMAnalyzer(cconfig);
        idmlib::util::IDMAnalyzerConfig csconfig = idmlib::util::IDMAnalyzerConfig::GetCommonConfig("","", "");
        csconfig.symbol = true;
        chars_analyzer_ = new idmlib::util::IDMAnalyzer(csconfig);
        std::string cidset_path = path_+"/cid_set";
        izenelib::am::ssf::Util<>::Load(cidset_path, cid_set_);
        std::string products_path = path_+"/products";
        izenelib::am::ssf::Util<>::Load(products_path, products_);
        std::string a2p_path = path_+"/a2p";
        izenelib::am::ssf::Util<>::Load(a2p_path, a2p_);
        std::string category_group_file = path_+"/category_group";
        if(boost::filesystem::exists(category_group_file))
        {
            LoadCategoryGroup(category_group_file);
        }
        std::string category_keywords_file = path_+"/category_keywords";
        if(boost::filesystem::exists(category_keywords_file))
        {
            LoadCategoryKeywords_(category_keywords_file);
        }
        std::string category_file = path_+"/category.txt";
        if(boost::filesystem::exists(category_file))
        {
            LoadCategories_(category_file);
        }
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

    for(uint32_t i=0;i<runtime_path.size();i++)
    {
        boost::filesystem::remove_all(path+"/"+runtime_path[i]);
    }
}
void ProductMatcher::InsertCategoryGroup(const std::vector<std::string>& group)
{
    if(group.size()<2) return;
    std::string s = group[0];
    for(uint32_t i=1;i<group.size();i++)
    {
        category_group_[group[i]] = s;
    }
}

void ProductMatcher::LoadCategoryGroup(const std::string& file)
{
    category_group_.clear();
    std::ifstream ifs(file.c_str());
    std::string line;
    std::vector<std::string> group;
    while( getline(ifs, line))
    {
        boost::algorithm::trim(line);
        if(line.empty()&&!group.empty())
        {
            InsertCategoryGroup(group);
            group.resize(0);
        }
        if(!line.empty())
        {
            group.push_back(line);
        }
    }
    if(!group.empty())
    {
        InsertCategoryGroup(group);
    }
}

void ProductMatcher::LoadCategoryKeywords_(const std::string& file)
{
    edit_category_keywords_.clear();
    std::ifstream ifs(file.c_str());
    std::string line;
    while( getline(ifs, line))
    {
        boost::algorithm::trim(line);
        std::vector<std::string> tokens;
        boost::algorithm::split( tokens, line, boost::algorithm::is_any_of(",") );
        if(tokens.size()<1) continue;
        std::string token = tokens[0];
        //if(category_keywords.find(token)!=category_keywords.end()) continue;
        //category_keywords.insert(token);
        edit_category_keywords_.push_back(token);
        LOG(INFO)<<"find category keyword "<<token<<std::endl;
    }
    ifs.close();
}

void ProductMatcher::LoadCategories_(const std::string& file)
{
    category_list_.clear();
    std::ifstream ifs(file.c_str());
    std::string line;
    while( getline(ifs, line))
    {
        boost::algorithm::trim(line);
        std::vector<std::string> tokens;
        boost::algorithm::split( tokens, line, boost::algorithm::is_any_of(",") );
        if(tokens.size()!=4) continue;
        SpuCategory spu_category;
        spu_category.scategory = tokens[0];
        std::string is_parent = tokens[3];
        if(is_parent=="true")
        {
            spu_category.is_parent = true;
        }
        else
        {
            spu_category.is_parent = false;
        }
        spu_category.cid = boost::lexical_cast<uint32_t>(tokens[1]);
        spu_category.parent_cid = boost::lexical_cast<uint32_t>(tokens[2]);
        string_similarity_.Convert(spu_category.scategory, spu_category.obj);
        category_index_[spu_category.scategory] = category_list_.size();
        category_list_.push_back(spu_category);
    }
    ifs.close();
}

bool ProductMatcher::Index(const std::string& scd_path)
{
    if(!a2p_.empty())
    {
        std::cout<<"product trained at "<<path_<<std::endl;
        return true;
    }
    std::string from_file = scd_path+"/category_group";
    std::string to_file = path_+"/category_group";
    if(boost::filesystem::exists(from_file))
    {
        if(boost::filesystem::exists(to_file))
        {
            boost::filesystem::remove_all(to_file);
        }
        boost::filesystem::copy_file(from_file, to_file);
        LoadCategoryGroup(to_file);
    }
    from_file = scd_path+"/category_keywords";
    to_file = path_+"/category_keywords";
    if(boost::filesystem::exists(from_file))
    {
        if(boost::filesystem::exists(to_file))
        {
            boost::filesystem::remove_all(to_file);
        }
        boost::filesystem::copy_file(from_file, to_file);
        LoadCategoryKeywords_(to_file);
    }
    from_file = scd_path+"/category.txt";
    to_file = path_+"/category.txt";
    if(boost::filesystem::exists(from_file))
    {
        if(boost::filesystem::exists(to_file))
        {
            boost::filesystem::remove_all(to_file);
        }
        boost::filesystem::copy_file(from_file, to_file);
        LoadCategories_(to_file);
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
    ScdParser parser(izenelib::util::UString::UTF_8);
    parser.load(scd);
    uint32_t n=0;
    a2p_.clear();
    products_.resize(1);
    AttribId aid = 0;
    for( ScdParser::iterator doc_iter = parser.begin();
      doc_iter!= parser.end(); ++doc_iter, ++n)
    {
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
        CategoryGroup::const_iterator it = category_group_.find(scategory);
        if(it!=category_group_.end())
        {
            //std::cerr<<"category convert "<<scategory<<" -> "<<it->second<<std::endl;
            scategory = it->second;
            category = UString(scategory, UString::UTF_8);
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
        std::string aid_str;
        uint32_t cid = GetCategoryId_(category);
        cid_set_.insert(cid);
        //std::string scategory;
        //category.convertString(scategory, izenelib::util::UString::UTF_8);
        //if( !cs_matcher_.Match(scategory) )
        //{
            //continue;
        //}
        std::string stitle;
        title.convertString(stitle, izenelib::util::UString::UTF_8);
        std::string soid;
        oid.convertString(soid, UString::UTF_8);
        Product product;
        product.spid = soid;
        product.stitle = stitle;
        product.scategory = scategory;
        product.price = price;
        products_.push_back(product);
        //std::cerr<<"[SPU][Title]"<<stitle<<std::endl;
        std::vector<AttrPair> attrib_list;
        std::vector<std::pair<UString, std::vector<UString> > > my_attrib_list;
        split_attr_pair(attrib_ustr, attrib_list);
        for(std::size_t i=0;i<attrib_list.size();i++)
        {
            const std::vector<izenelib::util::UString>& attrib_value_list = attrib_list[i].second;
            if(attrib_value_list.size()!=1) continue; //ignore empty value attrib and multi value attribs
            izenelib::util::UString attrib_value = attrib_value_list[0];
            izenelib::util::UString attrib_name = attrib_list[i].first;
            if(attrib_value.length()==0 || attrib_value.length()>30) continue;
            std::vector<std::string> value_list;
            std::string svalue;
            attrib_value.convertString(svalue, izenelib::util::UString::UTF_8);
            boost::algorithm::split(value_list, svalue, boost::algorithm::is_any_of("/"));
            //if(value_list.size()!=2)
            //{
                //value_list.resize(1);
                //value_list[0] = svalue;
            //}
            std::vector<UString> my_value_list;
            for(std::size_t j=0;j<value_list.size();j++)
            {
                UString value(value_list[j], UString::UTF_8);
                std::vector<izenelib::util::UString> termstr_list;
                AnalyzeChar_(value, termstr_list);
                value.clear();
                for(uint32_t t=0;t<termstr_list.size();t++)
                {
                    value.append(termstr_list[t]);
                }
                my_value_list.push_back(value);

                //boost::algorithm::to_lower(value_list[j]);
                //my_value_list.push_back(UString(value_list[j], UString::UTF_8));
            }
            if(!my_attrib_list.empty())
            {
                const std::vector<UString>& last_value = my_attrib_list.back().second;
                std::vector<std::string> s_last_value(last_value.size());
                for(uint32_t j=0;j<last_value.size();j++)
                {
                    last_value[j].convertString(s_last_value[j], UString::UTF_8);
                }
                std::vector<UString> tmp;
                for(uint32_t j=0;j<my_value_list.size();j++)
                {
                    std::string s_my_value;
                    my_value_list[j].convertString(s_my_value, UString::UTF_8);
                    for(uint32_t k=0;k<s_last_value.size();k++)
                    {
                        if(boost::algorithm::starts_with(s_my_value, s_last_value[k]))
                        {
                            //std::cout<<"before "<<s_my_value<<std::endl;
                            s_my_value = s_my_value.substr(s_last_value[k].size());
                            //std::cout<<"after "<<s_my_value<<std::endl;
                            break;
                        }
                    }
                    if(!s_my_value.empty())
                    {
                        tmp.push_back(UString(s_my_value, UString::UTF_8));
                    }
                }
                tmp.swap(my_value_list);
            }
            if(!my_value_list.empty())
            {
                my_attrib_list.push_back(std::make_pair(attrib_name, my_value_list));
            }
        }
        if(my_attrib_list.size()<2)
        {
            //std::cerr<<"igore: "<<stitle<<std::endl;
            continue;
        }
        uint32_t attrib_count = my_attrib_list.size();
        double weight = 1.0/attrib_count;
        for(std::size_t i=0;i<my_attrib_list.size();i++)
        {
            const std::vector<UString>& value = my_attrib_list[i].second;
            AttribId my_aidc = GenAID_(category, value, aid);
            UString empty_category;
            AttribId my_aid = GenAID_(empty_category, value, aid);
            //CAttribId caid = GetCAID_(my_aid);
            //CAttribId caidc = GetCAID_(my_aid, category);
#ifdef B5M_DEBUG
            //std::cout<<"before a2p insert "<<caid<<" : "<<a2p_[caid].size()<<std::endl;
            //std::cout<<"before a2pc insert "<<caidc<<" : "<<a2p_[caidc].size()<<std::endl;
            //for(uint32_t tmp=0;tmp<a2p_[caidc].size();tmp++)
            //{
                //std::cout<<a2p_[caidc][tmp].first<<","<<a2p_[caidc][tmp].second<<std::endl;
            //}
            //std::cout<<"a2pc inserting "<<caidc<<" : "<<piid<<","<<weight<<std::endl;
#endif
            //a2p_[caid].push_back(std::make_pair(piid, weight));
            //a2p_[caidc].push_back(std::make_pair(piid, weight));
            a2p_[my_aidc].push_back(std::make_pair(piid, weight));
            a2p_[my_aid].push_back(std::make_pair(piid, weight));
        }
    }
    std::string cidset_path = path_+"/cid_set";
    izenelib::am::ssf::Util<>::Save(cidset_path, cid_set_);
    std::string products_path = path_+"/products";
    izenelib::am::ssf::Util<>::Save(products_path, products_);
    std::string a2p_path = path_+"/a2p";
    izenelib::am::ssf::Util<>::Save(a2p_path, a2p_);
    WriteCategoryGroup_();
    
    return true;
}

void ProductMatcher::WriteCategoryGroup_()
{
    std::map<std::string, std::vector<std::string> > tmp;
    for(CategoryGroup::const_iterator it = category_group_.begin(); it!=category_group_.end(); ++it)
    {
        tmp[it->second].push_back(it->first);
    }
    std::string category_group_file = path_+"/category_group";
    std::ofstream ofs(category_group_file.c_str());
    for(std::map<std::string, std::vector<std::string> >::iterator it = tmp.begin();it!=tmp.end();++it)
    {
        ofs<<it->first<<std::endl;
        std::sort(it->second.begin(), it->second.end());
        for(uint32_t i=0;i<it->second.size();i++)
        {
            ofs<<it->second[i]<<std::endl;
        }
        ofs<<std::endl;
    }
    ofs.close();
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
                LOG(INFO)<<"Find Product Documents "<<n<<std::endl;
            }
            SCDDoc& scddoc = *(*doc_iter);
            SCDDoc::iterator p = scddoc.begin();
            Document doc;
            for(; p!=scddoc.end(); ++p)
            {
                const std::string& property_name = p->first;
                doc.property(property_name) = p->second;
            }
            izenelib::util::UString title;
            izenelib::util::UString category;
            doc.getProperty("Category", category);
            doc.getProperty("Title", title);
            
            if(category.length()==0 || title.length()==0)
            {
                continue;
            }
            Product product;
            if(GetMatched(doc, product))
            {
                UString oid;
                doc.getProperty("DOCID", oid);
                std::string soid;
                oid.convertString(soid, izenelib::util::UString::UTF_8);
                ofs<<soid<<","<<product.spid;
                boost::posix_time::ptime now = boost::posix_time::second_clock::local_time();
                ofs<<","<<boost::posix_time::to_iso_string(now);
                std::string stitle;
                title.convertString(stitle, UString::UTF_8);
                ofs<<","<<stitle<<","<<product.stitle<<std::endl;
            }
            else
            {
                //std::cerr<<"not got pid"<<std::endl;
            }
        }
    }
    ofs.close();
    return true;
}

bool ProductMatcher::GetMatched(const Document& doc, Product& product)
{
    std::vector<Product> products;
    if(GetMatched(doc, 1, products) && !products.empty())
    {
        product = products[0];
        return true;
    }
    return false;
}

bool ProductMatcher::GetMatched(const Document& doc, uint32_t count, std::vector<Product>& products)
{
    UString category;
    UString text;
    double price = 0.0;
    UString uprice;
    if(doc.getProperty("Price", uprice))
    {
        ProductPrice pp;
        pp.Parse(uprice);
        pp.GetMid(price);
    }
    doc.getProperty("Category", category);
    doc.getProperty("Title", text);
    if(text.empty()) return false;

    UString udocid;
    doc.getProperty("DOCID", udocid);
    std::string sdocid;
    udocid.convertString(sdocid, UString::UTF_8);

    std::string stext;
    text.convertString(stext, UString::UTF_8);
    std::string scategory;
    category.convertString(scategory, UString::UTF_8);
    CategoryGroup::const_iterator it = category_group_.find(scategory);
    if(it!=category_group_.end())
    {
        //std::cerr<<"category convert "<<scategory<<" -> "<<it->second<<std::endl;
        scategory = it->second;
        category = UString(scategory, UString::UTF_8);
    }
    if(!category.empty())
    {
        if(boost::algorithm::starts_with(scategory, "书籍/杂志/报纸"))
        {
            std::string nattrib_name = "isbn";
            UString attrib_ustr;
            doc.getProperty("Attribute", attrib_ustr);
            if(attrib_ustr.empty()) return false;
            std::vector<AttrPair> attrib_list;
            split_attr_pair(attrib_ustr, attrib_list);
            UString attrib_for_match;
            for(std::size_t i=0;i<attrib_list.size();i++)
            {
                const std::vector<izenelib::util::UString>& attrib_value_list = attrib_list[i].second;
                if(attrib_value_list.empty()) continue; //ignore empty value attrib
                const izenelib::util::UString& attrib_value = attrib_value_list[0];
                std::string sattrib_name;
                attrib_list[i].first.convertString(sattrib_name, izenelib::util::UString::UTF_8);
                boost::to_lower(sattrib_name);
                boost::algorithm::trim(sattrib_name);
                if(sattrib_name==nattrib_name)
                {
                    attrib_for_match = attrib_value;
                    break;
                }
            }
            std::string sam;
            attrib_for_match.convertString(sam, izenelib::util::UString::UTF_8);
            //std::cout<<"find attrib value : "<<sam<<std::endl;
            boost::algorithm::replace_all(sam, "-", "");
            attrib_for_match = UString(sam, UString::UTF_8);
            if(attrib_for_match.length()==0) return false;
            {
                std::string sam;
                attrib_for_match.convertString(sam, izenelib::util::UString::UTF_8);
                //std::cout<<"find attrib value : "<<sam<<std::endl;
            }
            izenelib::util::UString pid_str("attrib-"+nattrib_name+"-", izenelib::util::UString::UTF_8);
            pid_str.append(attrib_for_match);
            uint128_t pid = izenelib::util::HashFunction<izenelib::util::UString>::generateHash128(pid_str);
            Product product;
            product.spid = B5MHelper::Uint128ToString(pid);
            products.push_back(product);
            return true;
        }
        uint32_t cid = GetCategoryId_(category);
        if(cid_set_.find(cid)==cid_set_.end())
        {
            return false;
        }
    }
    std::set<AttribId> aid_set;
    GetAttribIdSet(category, text, aid_set);
    if(sdocid==test_docid_)
    {
        std::cout<<"!!!!show debug aid"<<std::endl;
        std::cout<<"text:"<<stext<<std::endl;
        std::cout<<"category:"<<scategory<<std::endl;
        for(std::set<AttribId>::iterator ait = aid_set.begin(); ait!=aid_set.end(); ++ait)
        {
            AttribId aid = *ait;
            UString name;
            aid_manager_->getDocNameByDocId(aid, name);
            std::string sname;
            name.convertString(sname, UString::UTF_8);
            std::cout<<"XXX "<<aid<<","<<sname<<std::endl;
        }
    }
    if(aid_set.empty()) return false;
    
    typedef std::map<PidType, double> PidMap;
    PidMap pid_map;
    for(std::set<AttribId>::iterator it = aid_set.begin();it!=aid_set.end();it++)
    {
        AttribId aid = *it;
        //CAttribId caid = 0;
        //if(category.empty())
        //{
            //caid = GetCAID_(aid);
        //}
        //else
        //{
            //caid = GetCAID_(aid, category);
        //}
        A2PMap::iterator pit = a2p_.find(aid);
        if(pit!=a2p_.end())
        {
            PidList& plist = pit->second;
            std::set<PidType> pid_exist;
            for(uint32_t i=0;i<plist.size();i++)
            {
                PidType this_pid = plist[i].first;
                if(pid_exist.find(this_pid)!=pid_exist.end()) continue;
                double weight = plist[i].second;
#ifdef B5M_DEBUG
                std::cout<<"aid,caid,pid,weight "<<aid<<","<<caid<<","<<this_pid<<","<<weight<<std::endl;
#endif
                if(pid_map.find(this_pid)==pid_map.end())
                {
                    pid_map.insert(std::make_pair(this_pid, weight));
                }
                else
                {
                    pid_map[this_pid]+=weight;
                }
                pid_exist.insert(this_pid);
            }
        }
    }
    std::vector<std::pair<double, Product> > product_candidate;
    for(PidMap::iterator it = pid_map.begin();it!=pid_map.end();++it)
    {
        if(it->second>=1.0)
        {
            PidType pid = it->first;
            Product this_product;
            GetProductInfo(pid, this_product);
            if(price>0.0&&this_product.price>0.0)
            {
                if(!PriceMatch_(price, this_product.price))
                {
                    continue;
                }
            }
            double str_sim = string_similarity_.Sim(text, UString(this_product.stitle, UString::UTF_8));
            if(sdocid==test_docid_)
            {
                std::cerr<<"sim with "<<this_product.stitle<<" : "<<str_sim<<std::endl;
            }
#ifdef B5M_DEBUG
            std::cout<<"sim with "<<pid<<","<<this_product.stitle<<" : "<<str_sim<<std::endl;
#endif
            product_candidate.push_back(std::make_pair(str_sim, this_product));
        }
        else
        {
            PidType pid = it->first;
            Product this_product;
            GetProductInfo(pid, this_product);
            if(sdocid==test_docid_)
            {
                std::cerr<<"unmatch with "<<this_product.stitle<<" : "<<it->second<<std::endl;
            }
        }
    }
    if(!product_candidate.empty())
    {
        typedef izenelib::util::first_greater<std::pair<double, Product> > greater_than;
        std::sort(product_candidate.begin(), product_candidate.end(), greater_than());
        uint32_t pick = std::min(count, (uint32_t)product_candidate.size());
        for(uint32_t i=0;i<pick;i++)
        {
            products.push_back(product_candidate[i].second);
        }
        return true;
    }
    return false;
}

bool ProductMatcher::GetProductInfo(const PidType& pid, Product& product)
{
    if(pid<products_.size())
    {
        product = products_[pid];
        return true;
    }
    return false;
}

void ProductMatcher::CRInit_()
{
    const std::vector<SpuCategory>& spu_categories = category_list_;
    for(uint32_t i=0;i<spu_categories.size();i++)
    {
        if(i%1000==0)
        {
            LOG(INFO)<<"scanning category "<<i<<std::endl;
        }
        const SpuCategory& spu_category = spu_categories[i];
        if(spu_category.is_parent) continue;
        const std::string& category = spu_category.scategory;
        if(category.empty()) continue;
        uint32_t category_index = i;
        std::vector<std::string> cs_list;
        boost::algorithm::split( cs_list, category, boost::algorithm::is_any_of(">") );
        if(cs_list.empty()) continue;
        boost::unordered_set<std::string> app;
        
        for(uint32_t c=0;c<cs_list.size();c++)
        {
            double score = 1.0/(cs_list.size()-c);
            std::string cs = cs_list[c];
            std::vector<std::string> words;
            boost::algorithm::split( words, cs, boost::algorithm::is_any_of("/") );
            for(uint32_t i=0;i<words.size();i++)
            {
                std::string w = words[i];
                std::vector<std::string> keyword_list(1, w);
                for(EditCategoryKeywords::const_iterator kit=edit_category_keywords_.begin();kit!=edit_category_keywords_.end();kit++)
                {
                    std::string keyword = *kit;
                    
                    if(keyword!=w&&(boost::algorithm::ends_with(w, keyword)||boost::algorithm::starts_with(w, keyword)))
                    {
                        keyword_list.push_back(keyword);
                    }
                }
                for(uint32_t k=0;k<keyword_list.size();k++)
                {
                    std::string keyword = keyword_list[k];
                    TrieType::iterator it = trie_.find(keyword);
                    if(it==trie_.end())
                    {
                        CategoryScoreList csl(1, std::make_pair(category_index, score));
                        trie_.insert(std::make_pair(keyword, csl));
                    }
                    else
                    {
                        CategoryScoreList& csl = it->second;
                        if(csl.back().first==category_index)
                        {
                            if(score>csl.back().second)
                            {
                                csl.back().second = score;
                            }
                        }
                        else
                        {
                            csl.push_back(std::make_pair(category_index, score));
                        }
                    }
                }
            }
        }
    }
    ss_objects_.resize(products_.size());
    for(uint32_t i=0;i<products_.size();i++)
    {
        string_similarity_.Convert(UString(products_[i].stitle, UString::UTF_8), ss_objects_[i]);
    }
    if(cr_result_==NULL)
    {
        std::string dir = path_+"/cr_result";
        cr_result_ = new CRResult(dir);
        cr_result_->open();
    }
}
bool ProductMatcher::GetCategory(const Document& doc, UString& category)
{
    static const uint32_t max_ngram_len = 8;
    if(trie_.empty())
    {
        CRInit_();
    }
    std::string sdocid;
    doc.getString("DOCID", sdocid);
    if(cr_result_->get(sdocid, category))
    {
        return true;
    }
    UString title;
    doc.getProperty("Title", title);
    if(title.empty()) return false;
    typedef boost::unordered_map<uint32_t, TitleCategoryScore> CandidatesMap;
    CandidatesMap candidates_map;
    //CategorySS category_ss;
    //GetCategorySS_(title, category_ss);
    std::string stitle;
    title.convertString(stitle, UString::UTF_8);
    std::vector<UString> tokens;
    AnalyzeCR_(title, tokens);
    //typedef boost::unordered_map<std::string, double> CategoryScoreMap;
    //CategoryScoreMap cs_map;
    //typedef std::pair<std::string, CategoryScoreList> Candidate;
    //std::vector<Candidate> candidates;
    //LOG(INFO)<<"[T] "<<stitle<<std::endl;
    uint32_t i=0;
    uint8_t bracket_depth = 0;
    while(i<tokens.size())
    {
        std::string stoken;
        tokens[i].convertString(stoken, UString::UTF_8);
        if(stoken==left_bracket_)
        {
            bracket_depth++;
            i++;
            continue;
        }
        else if(stoken==right_bracket_)
        {
            if(bracket_depth>0)
            {
                bracket_depth--;
            }
            i++;
            continue;
        }
        
        uint32_t max_len = std::min((uint32_t)tokens.size()-i, max_ngram_len);
        //Candidate candidate;
        uint32_t clen = 0;
        //bool cfound = false;
        std::pair<std::string, uint32_t> found("", 0); //ngram, position
        for(uint32_t len=1;len<=max_len;++len)
        {
            bool meet_symbol = false;
            UString ustr;
            for(uint32_t j=i;j<i+len;j++)
            {
                std::string stoken;
                tokens[j].convertString(stoken, UString::UTF_8);
                if(stoken==left_bracket_||stoken==right_bracket_)
                {
                    meet_symbol = true;
                    break;
                }
                ustr.append(tokens[j]);
            }
            if(meet_symbol) break;
            std::string str;
            ustr.convertString(str, UString::UTF_8);
            //LOG(INFO)<<"[N] "<<str<<std::endl;
            TrieType::const_iterator it = trie_.find(str);
            if(it!=trie_.end())
            {
                found.first = str;
                found.second = i+len;//end position
                //double position_score = (double)i*i/10.0;
                //CategoryScoreList cs_list = it->second;
                //for(uint32_t c=0;c<cs_list.size();c++)
                //{
                    //const std::string& category = cs_list[c].first;
                    //double ss = 0.0;
                    //CategorySS::iterator css_it = category_ss.find(category);
                    //if(css_it==category_ss.end())
                    //{
                        //ss = 0.2;
                    //}
                    //else
                    //{
                        //ss = css_it->second;
                    //}
                    //double css_score = string_similarity_.Sim(stitle, category);
                    //double final_score = ss*css_score*position_score;
                    //LOG(INFO)<<"[SS] "<<category<<" : "<<final_score<<std::endl;
                    //cs_list[c].second = final_score; //set related to position
                //}
                //candidate = std::make_pair(str, cs_list);
                clen = len;
                //cfound = true;
            }
        }
        if(found.first.length()>0) //found candidate ngram
        {
            uint32_t position = found.second;
            static const uint32_t length_smooth = 2;
            double position_score = (double)(length_smooth+position)/(tokens.size()+length_smooth);
            if(bracket_depth>0)
            {
                //LOG(INFO)<<"found in bracket "<<found.first<<std::endl;
                position_score*=0.2;
            }
            else
            {
                //LOG(INFO)<<"found normal "<<found.first<<std::endl;
            }
            TitleCategoryScore tcs;
            tcs.position = position_score;
            const CategoryScoreList& cs_list = trie_[found.first];
            for(uint32_t c=0;c<cs_list.size();c++)
            {
                //const std::string& category = cs_list[c].first;
                tcs.category_index = cs_list[c].first;
                candidates_map[tcs.category_index] = tcs;
                //LOG(INFO)<<"[SS] "<<found.first<<","<<category<<","<<position_score<<std::endl;
                //double ss = 0.0;
                //CategorySS::iterator css_it = category_ss.find(category);
                //if(css_it==category_ss.end())
                //{
                    //ss = 0.2;
                //}
                //else
                //{
                    //ss = css_it->second;
                //}
                //double css_score = string_similarity_.Sim(stitle, category);
                //double final_score = ss*css_score*position_score;
                //LOG(INFO)<<"[SS] "<<category<<" : "<<final_score<<std::endl;
                //cs_list[c].second = final_score; //set related to position
            }
            //candidates.push_back(candidate);
            i+=1;
            //i+=clen;
        }
        else
        {
            i+=1;
        }
    }
    Product product;
    if(GetMatched(doc, product))
    {
        //LOG(INFO)<<"[MATCH]"<<product.scategory<<","<<product.stitle<<std::endl;
        const std::string& category = product.scategory;
        uint32_t category_index = category_index_[category];
        TitleCategoryScore& tcs = candidates_map[category_index];
        tcs.category_index = category_index;
        tcs.position = 1.1;
        tcs.category_sim = 0.2;
        //tcs.spu_sim = 1.0;
        //candidates_map[tcs.scategory] = tcs;
        //CategoryScoreList cs_list(1, std::make_pair(product.scategory, 1.0));
        //candidates.push_back(std::make_pair(stitle, cs_list));
    }
    //std::pair<std::string, double> cmatch("", 0.0);
    std::pair<TitleCategoryScore, double> cmatch(TitleCategoryScore(), 0.0);
    SimObject title_obj;
    string_similarity_.Convert(stitle, title_obj);
    for(CandidatesMap::iterator it = candidates_map.begin();it!=candidates_map.end();it++)
    {
        TitleCategoryScore& tcs = it->second;
        const SpuCategory& category = category_list_[tcs.category_index];
        if(tcs.category_sim<0.0)
        {
            tcs.category_sim = string_similarity_.Sim(title_obj, category.obj);
        }
        if(tcs.spu_sim<0.0)
        {
            tcs.spu_sim = ComputeSpuSim_(title_obj, category.scategory);
        }
        if(tcs.category_sim<=0.0) tcs.category_sim = 0.01;
        if(tcs.spu_sim<=0.0) tcs.spu_sim = 0.01;
        double final_score = tcs.position*tcs.category_sim*tcs.spu_sim*tcs.spu_sim;
        //LOG(INFO)<<"[CAND]"<<category.scategory<<","<<final_score<<",["<<tcs.position<<","<<tcs.category_sim<<","<<tcs.spu_sim<<"]"<<std::endl;
        if(final_score>cmatch.second)
        {
            cmatch.first = tcs;
            cmatch.second = final_score;
        }
    }
    
    if(cmatch.second>0.0)
    {
        TitleCategoryScore tcs = cmatch.first;
        const SpuCategory& c = category_list_[tcs.category_index];
        category = UString(c.scategory, UString::UTF_8);
        cr_result_->update(sdocid, category);
        //LOG(INFO)<<"[CR]"<<sdocid<<","<<stitle<<","<<c.scategory<<","<<cmatch.second<<",["<<tcs.position<<","<<tcs.category_sim<<","<<tcs.spu_sim<<"]"<<std::endl;
        return true;
    }
    else
    {
        //LOG(INFO)<<"[CR-NOTFOUND]"<<sdocid<<","<<stitle<<std::endl;
        return false;
    }
}

bool ProductMatcher::DoCR(const std::string& scd_path)
{
    //UString s1("我是中国人", UString::UTF_8);
    //UString s2("盖中盖", UString::UTF_8);
    //LOG(INFO)<<string_similarity_.Sim(s1, s2)<<std::endl;
    //return true;
    using namespace __gnu_pbds;
    const std::vector<SpuCategory>& spu_categories = category_list_;
    TrieType trie;
    //GenerateCategoryTokens_(spu_categories);
    //typedef boost::unordered_set<std::string> CategoryKeywords;
    //CategoryKeywords category_keywords;
    //for(uint32_t i=0;i<spu_categories.size();i++)
    //{
        //const SpuCategory& spu_category = spu_categories[i];
        //if(spu_category.is_parent) continue;
        //const std::string& category = spu_category.scategory;
        //if(category.empty()) continue;
        //std::vector<std::string> cs_list;
        //boost::algorithm::split( cs_list, category, boost::algorithm::is_any_of(">") );
        //if(cs_list.empty()) continue;
        ////std::string last_cs = cs_list.back();
        
        //for(uint32_t c=0;c<cs_list.size();c++)
        //{
            //std::string cs = cs_list[c];
            //std::vector<std::string> tokens;
            //boost::algorithm::split( tokens, cs, boost::algorithm::is_any_of("/") );
            //for(uint32_t i=0;i<tokens.size();i++)
            //{
                //std::string token = tokens[i];
                //category_keywords.insert(token);
            //}
        //}
    //}


    std::vector<std::string> scd_list;
    B5MHelper::GetIUScdList(scd_path, scd_list);
    if(scd_list.empty()) return false;
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
            if(n%100==0)
            {
                LOG(INFO)<<"Find Documents "<<n<<std::endl;
            }
            SCDDoc& scddoc = *(*doc_iter);
            SCDDoc::iterator p = scddoc.begin();
            Document doc;
            for(; p!=scddoc.end(); ++p)
            {
                const std::string& property_name = p->first;
                doc.property(property_name) = p->second;
            }
            izenelib::util::UString title;
            izenelib::util::UString category;
            doc.getProperty("Category", category);
            doc.getProperty("Title", title);
            
            if(title.empty()||!category.empty())
            {
                continue;
            }
            std::string stitle;
            title.convertString(stitle, UString::UTF_8);
            if(GetCategory(doc, category))
            {
                //std::string scategory;
                //category.convertString(scategory, UString::UTF_8);
                //LOG(INFO)<<"[CR]"<<stitle<<","<<scategory<<std::endl;
            }
            else
            {
                //LOG(INFO)<<"[CR-NOTFOUND]"<<stitle<<std::endl;
            }
            //std::vector<TitleCategoryScore> candidates;


            //std::vector<Product> products;
            //if(GetMatched(doc, 5, products))
            //{
                //typedef boost::unordered_map<std::string, uint32_t> KNN;
                //std::pair<std::string, uint32_t> result("", 0);
                //KNN knn;
                //for(uint32_t i=0;i<products.size();i++)
                //{
                    //const std::string& category = products[i].scategory;
                    //KNN::iterator it = knn.find(category);
                    //uint32_t count = 0;
                    //if(it==knn.end())
                    //{
                        //count = 1;
                        //knn.insert(std::make_pair(category, count));
                    //}
                    //else
                    //{
                        //it->second+=1;
                        //count = it->second;
                    //}
                    //if(count>result.second)
                    //{
                        //result.first = category;
                    //}
                //}
                //LOG(INFO)<<"[CR-"<<products.size()<<"] "<<stitle<<" -> "<<result.first<<std::endl;
            //}
            //else
            //{
                //LOG(INFO)<<"[CR-NOTFOUND] "<<stitle<<std::endl;
            //}
        }
    }
    return true;
}

//bool ProductMatcher::DoCR(const std::string& scd_path)
//{
    //using namespace __gnu_pbds;
    //typedef boost::unordered_map<std::string, uint16_t> CategorySI;
    //typedef boost::unordered_map<uint16_t, std::string> CategoryIS;
    //CategorySI category_si;
    //CategoryIS category_is;
    //typedef std::pair<uint16_t, double> CategoryScore;
    //typedef std::vector<CategoryScore> CategoryScoreList;
    //typedef uint64_t hash_t;
    ////typedef trie<std::string, CategoryScoreList> TrieType;
    //typedef boost::unordered_map<hash_t, CategoryScoreList> TrieType;
    //TrieType trie;
    //std::string cr_file = path_+"/cr_output";
    //std::ifstream ifs(cr_file.c_str());
    //std::string line;
    //uint16_t ucid = 1;
    //uint64_t p=0;
    //while( getline(ifs, line))
    //{
        //++p;
        //if(p%100000==0)
        //{
            //LOG(INFO)<<"Loading cr "<<p<<std::endl;
        //}
        //boost::algorithm::trim(line);
        //std::vector<std::string> tokens;
        //boost::algorithm::split( tokens, line, boost::algorithm::is_any_of(",") );
        //if(tokens.size()<5) continue;
        //std::string text = tokens[0];
        //uint32_t tf = boost::lexical_cast<uint32_t>(tokens[1]);
        //if(tf<5) continue;
        //CategoryScoreList cs_list;
        //for(uint32_t i=4;i<tokens.size();i++)
        //{
            //std::vector<std::string> vec;
            //boost::algorithm::split(vec, tokens[i], boost::algorithm::is_any_of(":"));
            //std::string category = vec[0];
            //double score = boost::lexical_cast<double>(vec[1])*std::log(tf*1.0);
            //uint16_t cid = 0;
            //CategorySI::iterator it = category_si.find(category);
            //if(it==category_si.end())
            //{
                //cid = ucid;
                //++ucid;
                //category_si[category] = cid;
                //category_is[cid] = category;
            //}
            //else
            //{
                //cid = it->second;
            //}
            //cs_list.push_back(std::make_pair(cid, score));
        //}
        ////trie[text] = cs_list;
        //hash_t hash = izenelib::util::HashFunction<UString>::generateHash64(UString(text,UString::UTF_8));
        //trie[hash] = cs_list;
    //}
    //ifs.close();

    //std::vector<std::string> scd_list;
    //B5MHelper::GetIUScdList(scd_path, scd_list);
    //if(scd_list.empty()) return false;
    //static const uint32_t max_ngram_len = 8;
    //for(uint32_t i=0;i<scd_list.size();i++)
    //{
        //std::string scd_file = scd_list[i];
        //LOG(INFO)<<"Processing "<<scd_file<<std::endl;
        //ScdParser parser(izenelib::util::UString::UTF_8);
        //parser.load(scd_file);
        //uint32_t n=0;
        //for( ScdParser::iterator doc_iter = parser.begin();
          //doc_iter!= parser.end(); ++doc_iter, ++n)
        //{
            //if(n%100000==0)
            //{
                //LOG(INFO)<<"Find Documents "<<n<<std::endl;
            //}
            //SCDDoc& scddoc = *(*doc_iter);
            //SCDDoc::iterator p = scddoc.begin();
            //Document doc;
            //for(; p!=scddoc.end(); ++p)
            //{
                //const std::string& property_name = p->first;
                //doc.property(property_name) = p->second;
            //}
            //izenelib::util::UString title;
            //izenelib::util::UString category;
            //doc.getProperty("Category", category);
            //doc.getProperty("Title", title);
            
            //if(title.empty()||!category.empty())
            //{
                //continue;
            //}
            //std::string stitle;
            //title.convertString(stitle, UString::UTF_8);
            //std::vector<UString> tokens;
            //AnalyzeChar_(title, tokens);
            //typedef boost::unordered_map<uint16_t, double> CategoryScoreMap;
            //CategoryScoreMap cs_map;
            //typedef std::pair<std::string, CategoryScoreList> Candidate;
            //std::vector<Candidate> candidates;
            ////LOG(INFO)<<"[T] "<<stitle<<std::endl;
            //uint32_t i=0;
            //while(i<tokens.size())
            //{
                //uint32_t max_len = std::min((uint32_t)tokens.size()-i, max_ngram_len);
                //Candidate candidate;
                //bool cfound = false;
                //for(uint32_t len=2;len<=max_len;++len)
                //{
                    //UString ustr;
                    //for(uint32_t j=i;j<i+len;j++)
                    //{
                        //ustr.append(tokens[j]);
                    //}
                    //std::string str;
                    //ustr.convertString(str, UString::UTF_8);
                    ////LOG(INFO)<<"[N] "<<str<<std::endl;
                    //hash_t hash = izenelib::util::HashFunction<UString>::generateHash64(ustr);
                    //TrieType::const_iterator it = trie.find(hash);
                    //if(it!=trie.end())
                    //{
                        //candidate = std::make_pair(str, it->second);
                        //cfound = true;
                        //std::cerr<<"[N] "<<str<<","<<category_is[it->first];
                        //for(uint32_t j=0;j<it->second.size();j++)
                        //{
                            //uint16_t cid = it->second[j].first;
                            //double score = it->second[j].second;
                            //std::string category = category_is[cid];
                            //std::cerr<<","<<category<<":"<<score;
                        //}
                        //std::cerr<<std::endl;
                    //}
                //}
                //if(cfound)
                //{
                    //std::cerr<<"[F] "<<candidate.first<<std::endl;
                    //candidates.push_back(candidate);
                    //i+=candidate.first.size();
                //}
                //else
                //{
                    //i+=1;
                //}
            //}
            //std::pair<uint16_t, double> cmatch(0, 0.0);
            //for(uint32_t i=0;i<candidates.size();i++)
            //{
                //const std::string& str = candidates[i].first;
                //const CategoryScoreList& cs_list = candidates[i].second;
                //for(uint32_t c=0;c<cs_list.size();c++)
                //{
                    //uint16_t cid = cs_list[c].first;
                    //double score = cs_list[c].second*str.length();
                    //CategoryScoreMap::iterator cit = cs_map.find(cid);
                    //if(cit==cs_map.end())
                    //{
                        //cs_map.insert(std::make_pair(cid, score));
                    //}
                    //else
                    //{
                        //cit->second+=score;
                        //score = cit->second;
                    //}
                    //if(score>cmatch.second)
                    //{
                        //cmatch.second = score;
                        //cmatch.first = cid;
                    //}
                //}
            //}
            //if(cmatch.first>0)
            //{
                //std::string category = category_is[cmatch.first];
                //LOG(INFO)<<"[CR] "<<stitle<<" -> "<<category<<std::endl;
            //}
            //else
            //{
                //LOG(INFO)<<"[CR-NOTFOUND] "<<stitle<<std::endl;
            //}


            ////std::vector<Product> products;
            ////if(GetMatched(doc, 5, products))
            ////{
                ////typedef boost::unordered_map<std::string, uint32_t> KNN;
                ////std::pair<std::string, uint32_t> result("", 0);
                ////KNN knn;
                ////for(uint32_t i=0;i<products.size();i++)
                ////{
                    ////const std::string& category = products[i].scategory;
                    ////KNN::iterator it = knn.find(category);
                    ////uint32_t count = 0;
                    ////if(it==knn.end())
                    ////{
                        ////count = 1;
                        ////knn.insert(std::make_pair(category, count));
                    ////}
                    ////else
                    ////{
                        ////it->second+=1;
                        ////count = it->second;
                    ////}
                    ////if(count>result.second)
                    ////{
                        ////result.first = category;
                    ////}
                ////}
                ////LOG(INFO)<<"[CR-"<<products.size()<<"] "<<stitle<<" -> "<<result.first<<std::endl;
            ////}
            ////else
            ////{
                ////LOG(INFO)<<"[CR-NOTFOUND] "<<stitle<<std::endl;
            ////}
        //}
    //}
    //return true;
//}

bool ProductMatcher::PriceMatch_(double p1, double p2)
{
    static double min_ratio = 0.25;
    static double max_ratio = 3;
    double ratio = p1/p2;
    return (ratio>=min_ratio)&&(ratio<=max_ratio);
}

uint32_t ProductMatcher::GetCategoryId_(const UString& category)
{
    return izenelib::util::HashFunction<UString>::generateHash32(category);
}

AttribId ProductMatcher::GenAID_(const UString& category, const std::vector<UString>& value, AttribId& aid)
{
    bool find_my_aid = false;
    AttribId my_aid = 0;
    std::vector<UString> rep_list(value.size());
    for(std::size_t i=0;i<value.size();i++)
    {
        rep_list[i] = GetAttribRep_(category, value[i]);
    }
    for(std::size_t i=0;i<rep_list.size();i++)
    {
        if(aid_manager_->getDocIdByDocName(rep_list[i], my_aid))
        {
            find_my_aid = true;
            break;
        }
    }
    if(!find_my_aid)
    {
        ++aid;
        my_aid = aid;
    }
    for(std::size_t i=0;i<rep_list.size();i++)
    {
        aid_manager_->Set(rep_list[i], my_aid);
        //std::string svalue;
        //rep_list[i].convertString(svalue, UString::UTF_8);
        //std::cerr<<"[ATTR]"<<svalue<<","<<my_aid<<std::endl;
    }
    return my_aid;
}

uint64_t ProductMatcher::GetCAID_(AttribId aid, const UString& category)
{
    uint32_t cid = GetCategoryId_(category);
    CAttribId caid = cid;
    caid = caid<<32;
    caid += aid;
    return caid;
}

uint64_t ProductMatcher::GetCAID_(AttribId aid)
{
    CAttribId caid = aid;
    return caid;
}
void ProductMatcher::NormalizeText_(const izenelib::util::UString& text, izenelib::util::UString& ntext)
{
    std::string str;
    text.convertString(str, izenelib::util::UString::UTF_8);
    //logger_<<"[BNO]"<<str<<std::endl;
    boost::to_lower(str);
    //for(uint32_t i=0;i<normalize_pattern_.size();i++)
    //{
        //str = boost::regex_replace(str, normalize_pattern_[i].first, normalize_pattern_[i].second);
    //}
    //logger_<<"[ANO]"<<str<<std::endl;
    ntext = izenelib::util::UString(str, izenelib::util::UString::UTF_8);
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
    izenelib::util::UString text;
    NormalizeText_(btext, text);
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
    izenelib::util::UString text;
    NormalizeText_(btext, text);
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

UString ProductMatcher::GetAttribRep_(const UString& category, const UString& text)
{
    UString rep = category;
    rep.append(UString("|", UString::UTF_8));
    rep.append(text);
    return rep;
}

void ProductMatcher::GetAttribIdSet(const UString& category, const izenelib::util::UString& value, std::set<AttribId>& aid_set)
{
    static const uint32_t n = 10;
    static const uint32_t max_ngram_len = 20;
    static const uint32_t max_unmatch_count = 8;
    std::vector<izenelib::util::UString> termstr_list;
    AnalyzeChar_(value, termstr_list); //convert to lower case
    std::size_t index = 0;
#ifdef B5M_DEBUG
    std::string svalue;
    value.convertString(svalue, UString::UTF_8);
    std::cout<<"text: "<<svalue<<std::endl;
#endif
    while(index<termstr_list.size())
    {
        std::vector<AttribId> ngram_aid_list;
        izenelib::util::UString attrib_ngram;
        izenelib::util::UString ngram;
        std::size_t inc_len = 0;
        uint32_t unmatch_count = 0;
        for(std::size_t len = 0;len<n;len++)
        {
            std::size_t pos = index+len;
            if(pos>=termstr_list.size()) break;
            ngram.append(termstr_list[pos]);
            if(ngram.length()<2) continue;
            if(ngram.length()>max_ngram_len) break;
            std::vector<AttribId> aid_list;
            AttribId aid = 0;
            UString rep = GetAttribRep_(category, ngram);
            if(aid_manager_->getDocIdByDocName(rep, aid))
            {
                inc_len = len+1;
                attrib_ngram = ngram;
                ngram_aid_list.push_back(aid);
                unmatch_count = 0;
#ifdef B5M_DEBUG
                std::string sngram;
                ngram.convertString(sngram, UString::UTF_8);
                std::cout<<"find ngram aid "<<sngram<<","<<aid<<std::endl;
#endif
            }
            else
            {
                unmatch_count++;
                if(attrib_ngram.length()>0)//find one
                {
                    if(unmatch_count==max_unmatch_count)
                    {
                        break;
                    }
                }
            }
        }
        for(std::size_t i=0;i<ngram_aid_list.size();i++)
        {
            aid_set.insert(ngram_aid_list[i]);
        }
        if(attrib_ngram.length()>0)
        {
            index += inc_len;
        }
        else
        {
            ++index;
        }
    }
}

bool ProductMatcher::IndexCR()
{

    std::string dir = path_+"/kpe";
    std::string id_dir = dir+"/id";
    std::string tmp_dir = dir+"/tmp";
    std::string kp_dir = dir+"/kp";
    boost::filesystem::remove_all(dir);
    boost::filesystem::create_directories(dir);
    boost::filesystem::create_directories(tmp_dir);
    boost::filesystem::create_directories(id_dir);
    boost::filesystem::create_directories(kp_dir);
    std::string output_file = path_+"/cr_output";
    cr_ofs_.open(output_file.c_str());
    //IDMAnalyzer analyzer(idmlib::util::IDMAnalyzerConfig::GetCommonConfig("","",""));
    id_manager_ = new idmlib::util::IDMIdManager(id_dir);
    //uint8_t max_ngram_len = 8;

    izenelib::am::ssf::Writer<> ngram_writer(tmp_dir+"/ngram_writer");
    ngram_writer.Open();
    //uint32_t doc_count = 0;
    //uint32_t all_term_count = 0;
    for(uint32_t i=0;i<products_.size();i++)
    {
        uint32_t docid = i+1;
        if(docid%100000==0)
        {
            LOG(INFO)<<"Build CR for product "<<docid<<std::endl;
        }
        const std::string& scategory = products_[i].scategory;
        std::vector<UString> term_list;
        UString text(products_[i].stitle, UString::UTF_8);
        AnalyzeChar_(text, term_list);
        std::vector<TermInNgram> terms(term_list.size() );
        CategoryTermCount::iterator it = category_term_count_.find(scategory);
        if(it==category_term_count_.end())
        {
            category_term_count_.insert(std::make_pair(scategory, terms.size()));
        }
        else
        {
            it->second+=terms.size();
        }
        for(uint32_t t=0;t<term_list.size();t++)
        {
            
            uint32_t id = izenelib::util::HashFunction<izenelib::util::UString>::generateHash32(term_list[t]);
            id_manager_->Put(id, term_list[t]);
            terms[t].id = id;
            terms[t].tag = 'C';
        }
        for(uint32_t t=0;t<terms.size();t++)
        {
            uint32_t len = terms.size()-t;
            //if(len<2 || len>max_ngram_len) continue;
            std::vector<TermInNgram> new_term_list(terms.begin()+t, terms.begin()+t+len );
            Ngram ngram(new_term_list, docid);
            if( t!= 0 )
            {
                ngram.left_term = terms[t-1];
            }
            std::vector<uint32_t> list;
            ngram.ToUint32List(list);
            ngram_writer.Append(list);
            //LOG(INFO)<<"insert ngram size "<<ngram.term_list.size()<<std::endl;
            //Ngram parse_ngram = Ngram::ParseUint32List(list);
            //LOG(INFO)<<"parse ngram size "<<parse_ngram.term_list.size()<<std::endl;
        }
    }
    ngram_writer.Close();
    std::string ngram_path = ngram_writer.GetPath();
    izenelib::am::ssf::Sorter<uint32_t, uint32_t, true>::Sort(ngram_path);
    izenelib::am::ssf::Reader<> reader(ngram_path);
    if(!reader.Open()) return false;
    uint64_t p=0;
    std::vector<uint32_t> key_list;
    NgramInCollection last_ngram;
    std::vector<NgramInCollection> for_process;
    while( reader.Next(key_list) )
    {
        //std::cout<<"key-list: "<<std::endl;
        //for(uint32_t dd=0;dd<key_list.size();dd++)
        //{
            //std::cout<<key_list[dd]<<",";
        //}
        //std::cout<<std::endl;
        //if(!tracing_.empty())
        //{
            //if(VecStarts_(key_list, tracing_))
            //{
            //}
        //}
        Ngram ngram = Ngram::ParseUint32List(key_list);
        //LOG(INFO)<<"find ngram size "<<ngram.term_list.size()<<std::endl;
        if(last_ngram.IsEmpty())
        {
            last_ngram += ngram;
        }
        else
        {
            uint32_t inc = last_ngram.GetInc(ngram);
            if(inc==ngram.term_list.size()) //the same
            {
                last_ngram += ngram;
            }
            else
            {
                last_ngram.Flush();
                for_process.push_back(last_ngram);
                //LOG(INFO)<<"for_process push_back "<<last_ngram.term_list.size()<<std::endl;
                if(inc==0)
                {
                    NgramProcess_(for_process);
                    for_process.resize(0);
                }
                last_ngram.Clear();
                last_ngram.inc = inc;
                last_ngram += ngram;

            }
        }


        p++;
    }
    NgramProcess_(for_process);
    reader.Close();
    cr_ofs_.close();
    delete id_manager_;
    return true;

}

void ProductMatcher::NgramProcess_(const std::vector<NgramInCollection>& data)
{
    if( data.size()==0 ) return;
    //{
        //std::vector<TermInNgram> termList = data[0].term_list;
        //std::vector<uint32_t> termIdList(termList.size());
        //for(uint32_t i=0;i<termList.size();i++)
        //{
            //termIdList[i] = termList[i].id;
        //}
        //std::string ngram_str = GetNgramString_(termIdList);
        //if(ngram_str.find("jqutishow")!=0) return;
    //}
    //for( uint32_t m=0;m<data.size();m++)
    //{
        //std::vector<TermInNgram> termList = data[m].term_list;
        //std::vector<uint32_t> termIdList(termList.size());
        //for(uint32_t i=0;i<termList.size();i++)
        //{
            //termIdList[i] = termList[i].id;
        //}
        //std::vector<id2count_t> docItemList = data[m].docitem_list;
        //std::string ngram_str = GetNgramString_(termIdList);
        //for(uint32_t i=0;i<docItemList.size();i++)
        //{
            //uint32_t docid = docItemList[i].first;
            //const Product& product = products_[docid-1];
            //LOG(INFO)<<"ngram origin "<<data[m].inc<<" "<<ngram_str<<" in "<<docid<<","<<product.stitle<<std::endl;
        //}
    //}

    for( uint32_t m=0;m<data.size();m++)
    {
        //uint32_t last_len = 0;
        //if(m>0)
        //{
            //last_len = data[m-1].term_list.size();
        //}
        uint32_t pos = m;
        uint32_t inc = data[pos].inc;
        const std::vector<TermInNgram>& c_term_list = data[pos].term_list;
        for(uint32_t i=inc;i<c_term_list.size();i++)
        {
            NgramInCollection ngram;
            uint32_t len = i+1;
            ngram.term_list.assign(c_term_list.begin(), c_term_list.begin()+len);
            ngram+=data[pos];
            for(uint32_t n=m+1;n<data.size();n++)
            {
                if(data[n].inc<len) break;
                ngram+=data[n];
            }
            ngram.Flush();
            OutputNgram_(ngram);
        }
    }
    //sorting with postorder
    //std::vector<uint32_t> post_order(data.size());
    //{
        //std::stack<uint32_t> index_stack;
        //std::stack<uint32_t> depth_stack;
        //uint32_t i=0;
        //for( uint32_t m=0;m<data.size();m++)
        //{
            //uint32_t depth = data[m].inc;
            //if(m>0)
            //{
                //while( !depth_stack.empty() && depth <= depth_stack.top() )
                //{
                    //post_order[i] = index_stack.top();
                    //++i;

                    //index_stack.pop();
                    //depth_stack.pop();
                //}
            //}
            //index_stack.push(m);
            //depth_stack.push(depth);
        //}
        //while( !index_stack.empty() )
        //{
            //post_order[i] = index_stack.top();
            //++i;

            //index_stack.pop();
            //depth_stack.pop();
        //}
    //}

    //std::stack<uint32_t> depth_stack;
    //std::stack<uint32_t> freq_stack;
    //std::stack<std::vector<id2count_t> > doc_item_stack;
    //std::stack<std::vector<std::pair<TermInNgram, uint32_t> > > prefix_term_stack;
    //std::stack<std::pair<TermInNgram, uint32_t> > suffix_term_stack;
    //for( uint32_t m=0;m<data.size();m++)
    //{
        //uint32_t pos = post_order[m];
        //uint32_t depth = data[pos].inc;
        //std::vector<TermInNgram> termList = data[pos].term_list;
        //std::vector<id2count_t> docItemList = data[pos].docitem_list;
        //uint32_t freq = data[pos].freq;
        //std::vector<std::pair<TermInNgram, uint32_t> > prefixTermList = data[pos].lc_list;
        //std::vector<std::pair<TermInNgram, uint32_t> > suffixTermList;
        //while( !depth_stack.empty() && depth < depth_stack.top() )
        //{
            //freq+=freq_stack.top();
            //docItemList.insert( docItemList.end(), doc_item_stack.top().begin(), doc_item_stack.top().end() );
            //prefixTermList.insert( prefixTermList.end(), prefix_term_stack.top().begin(), prefix_term_stack.top().end() );
            ////suffix howto?
            ////suffixTermList, suffix_term_stack
    ////             suffixTermList.insert( suffixTermList.end(), suffix_term_stack.top().begin(), suffix_term_stack.top().end() );
            //suffixTermList.push_back(suffix_term_stack.top() );
            ////pop stack
            //depth_stack.pop();
            //freq_stack.pop();
            //doc_item_stack.pop();
            //prefix_term_stack.pop();
            //suffix_term_stack.pop();
        //}
        //depth_stack.push( depth);
        //freq_stack.push(freq);
        //doc_item_stack.push( docItemList);
        //prefix_term_stack.push(prefixTermList);
        ////get the suffix term
        //TermInNgram suffix_term = termList[depth];
        //suffix_term_stack.push(std::make_pair(suffix_term, freq) );
        ////std::cerr<<"term list size "<<termList.size()<<std::endl;

        ////std::cerr<<"[ENTROPY] "<<ngram_str<<" : "<<entropy<<std::endl;
        ////std::cerr<<"[MAXPROB] "<<ngram_str<<" : "<<max_prob<<std::endl;
        


    //}
}
std::string ProductMatcher::GetNgramString_(const std::vector<uint32_t>& termIdList)
{
    UString ngram_text;
    for(uint32_t i=0;i<termIdList.size();i++)
    {
        UString str;
        id_manager_->GetStringById(termIdList[i], str);
        ngram_text.append(str);
    }
    std::string ngram_str;
    ngram_text.convertString(ngram_str, UString::UTF_8);
    return ngram_str;
}

void ProductMatcher::OutputNgram_(const NgramInCollection& ngram)
{
    static const uint32_t max_ngram_len = 8;

    if( ngram.term_list.size() > max_ngram_len ) return;

    std::vector<uint32_t> termIdList(ngram.term_list.size());
    for(uint32_t i=0;i<termIdList.size();i++)
    {
        termIdList[i] = ngram.term_list[i].id;
    }

    std::string ngram_str = GetNgramString_(termIdList);
    typedef boost::unordered_map<std::string, uint32_t> CategoryApp;
    CategoryApp category_app;
    for(uint32_t i=0;i<ngram.docitem_list.size();i++)
    {
        uint32_t docid = ngram.docitem_list[i].first;
        uint32_t count = ngram.docitem_list[i].second;
        const Product& product = products_[docid-1];
        //LOG(INFO)<<"ngram "<<ngram_str<<" apps in "<<product.stitle<<std::endl;
        CategoryApp::iterator it = category_app.find(product.scategory);
        if(it==category_app.end())
        {
            category_app.insert(std::make_pair(product.scategory, count));
        }
        else
        {
            it->second+=count;
        }
    }
    typedef boost::unordered_map<std::string, double> CategoryProbMap;
    CategoryProbMap category_prob_map;
    double prob_sum = 0.0;
    for(CategoryTermCount::const_iterator it = category_term_count_.begin();it!=category_term_count_.end();++it)
    {
        double prob = 0.0;
        uint32_t category_term_count = it->second;
        CategoryApp::const_iterator ait = category_app.find(it->first);
        if(ait==category_app.end())
        {
            prob = 0.0;
        }
        else
        {
            uint32_t capp = ait->second;
            prob = (double)capp/category_term_count;
            //std::cerr<<"[CAPP] "<<ngram_str<<" in "<<it->first<<" : "<<capp<<","<<category_term_count<<std::endl;
        }
        category_prob_map[it->first] = prob;
        prob_sum += prob;
    }
    double r = 1.0/prob_sum;
    double entropy = 0.0;
    double max_prob = 0.0;
    for(CategoryProbMap::iterator it = category_prob_map.begin();it!=category_prob_map.end();++it)
    {
        it->second*=r;
        double prob = it->second;
        if(prob>max_prob)
        {
            max_prob = prob;
        }
        if(prob!=0.0)
        {
            entropy += prob*std::log(prob)/std::log(2.0);
        }
    }
    if(entropy<0.0) entropy*=-1.0;
    double ratio = entropy/max_prob;
    if(ratio<=4.0||entropy<2.0)
    {
        cr_ofs_<<ngram_str<<","<<ngram.freq<<","<<entropy<<","<<max_prob;
        for(CategoryProbMap::iterator it = category_prob_map.begin();it!=category_prob_map.end();++it)
        {
            if(it->second>=0.2)
            {
                cr_ofs_<<","<<it->first<<":"<<it->second;
            }
        }
        cr_ofs_<<std::endl;
    }
}

//void ProductMatcher::GetCategorySS_(UString title, CategorySS& ss)
//{
    //std::string stitle;
    //title.convertString(stitle, UString::UTF_8);
    //for(uint32_t i=0;i<products_.size();i++)
    //{
        //const Product& product = products_[i];
        //const idmlib::sim::StringSimilarity::Object& product_obj = ss_objects_[i];
        //idmlib::sim::StringSimilarity::Object obj;

        //string_similarity_.Convert(title, obj);
        //double score = string_similarity_.Sim(obj, product_obj);
        ////if(score>0.0)
        ////{
            ////LOG(INFO)<<"[SIM] "<<stitle<<","<<product.stitle<<" : "<<score<<std::endl;
        ////}
        //const std::string& scategory = product.scategory;
        //CategorySS::iterator it = ss.find(scategory);
        //if(it==ss.end())
        //{
            //ss.insert(std::make_pair(scategory, score));
        //}
        //else if(score>it->second)
        //{
            //it->second = score;
            //score = it->second;
        //}
        ////LOG(INFO)<<"[SIM] "<<stitle<<","<<product.scategory<<" : "<<score<<std::endl;
    //}
//}

void ProductMatcher::GenerateCategoryTokens_(const std::vector<SpuCategory>& spu_categories)
{
    typedef boost::unordered_set<std::string> CategoryTokenSet;
    CategoryTokenSet category_token_set;
    for(uint32_t i=0;i<spu_categories.size();i++)
    {
        const SpuCategory& spu_category = spu_categories[i];
        const std::string& category = spu_category.scategory;
        if(category.empty()) continue;
        std::vector<std::string> cs_list;
        boost::algorithm::split( cs_list, category, boost::algorithm::is_any_of(">") );
        if(cs_list.empty()) continue;
        //std::string last_cs = cs_list.back();
        
        for(uint32_t c=0;c<cs_list.size();c++)
        {
            std::string cs = cs_list[c];
            std::vector<std::string> tokens;
            boost::algorithm::split( tokens, cs, boost::algorithm::is_any_of("/") );
            for(uint32_t i=0;i<tokens.size();i++)
            {
                std::string token = tokens[i];
                category_token_set.insert(token);
            }
        }
    }
    typedef boost::unordered_map<std::string, std::pair<uint32_t, uint32_t> > CategoryNgramStat;
    CategoryNgramStat cngram_stat;
    for(uint32_t i=0;i<spu_categories.size();i++)
    {
        const SpuCategory& spu_category = spu_categories[i];
        const std::string& category = spu_category.scategory;
        if(category.empty()) continue;
        std::vector<std::string> cs_list;
        boost::algorithm::split( cs_list, category, boost::algorithm::is_any_of(">") );
        if(cs_list.empty()) continue;
        //std::string last_cs = cs_list.back();
        
        for(uint32_t c=cs_list.size()-1;c<cs_list.size();c++)
        {

            std::string cs = cs_list[c];
            std::vector<std::string> tokens;
            boost::algorithm::split( tokens, cs, boost::algorithm::is_any_of("/") );
            for(uint32_t i=0;i<tokens.size();i++)
            {
                std::string token = tokens[i];
                std::vector<UString> words;
                AnalyzeChar_(UString(token, UString::UTF_8), words);
                for(uint32_t w=0;w<words.size();w++)
                {
                    for(uint32_t e=w+1;e<=words.size();e++)
                    {
                        UString ungram;
                        for(uint32_t s=w;s<e;s++)
                        {
                            ungram.append(words[s]);
                        }
                        std::string ngram;
                        ungram.convertString(ngram, UString::UTF_8);
                        if(category_token_set.find(ngram)!=category_token_set.end()) continue;
                        uint32_t count_all = 1;
                        uint32_t count_edge = 0;
                        if(e==words.size()||w==0)
                        {
                            count_edge = 1;
                        }
                        std::pair<uint32_t, uint32_t> value(count_all, count_edge);
                        CategoryNgramStat::iterator it = cngram_stat.find(ngram);
                        if(it==cngram_stat.end())
                        {
                            cngram_stat.insert(std::make_pair(ngram, value));
                        }
                        else
                        {
                            value.first+=it->second.first;
                            value.second+=it->second.second;
                            it->second = value;
                        }
                    }
                }
            }
        }
    }
    std::string output_file = path_+"/category_token";
    std::ofstream ofs(output_file.c_str());
    for(CategoryNgramStat::const_iterator cit = cngram_stat.begin();cit!=cngram_stat.end();cit++)
    {
        const std::pair<uint32_t, uint32_t>& stat = cit->second;
        const std::string& ngram = cit->first;
        double ratio = (double)stat.second/stat.first;
        if(stat.first>=5 && ratio>=0.4)
        {
            ofs<<ngram<<","<<ratio<<","<<stat.first<<","<<stat.second<<std::endl;
        }
    }
    ofs.close();
}

double ProductMatcher::ComputeSpuSim_(const SimObject& obj, const std::string& scategory)
{
    std::vector<uint32_t> indexes;
    GetCategorySpuIndexes_(scategory, indexes);
    if(indexes.empty()) return 0.2;
    double max = 0.0;
    for(uint32_t i=0;i<indexes.size();i++)
    {
        double sim = string_similarity_.Sim(obj, ss_objects_[indexes[i]]);
        if(sim>max) max = sim;
    }
    return max;
}
void ProductMatcher::GetCategorySpuIndexes_(const std::string& scategory, std::vector<uint32_t>& product_indexes)
{
    if(category_spu_index_.empty())
    {
        for(uint32_t i=0;i<products_.size();i++)
        {
            category_spu_index_[products_[i].scategory].push_back(i);
        }
    }
    product_indexes = category_spu_index_[scategory];
}
