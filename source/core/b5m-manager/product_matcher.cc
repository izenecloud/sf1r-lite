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
#include <idmlib/similarity/string_similarity.h>
#include <util/functional.h>
using namespace sf1r;
using namespace idmlib::sim;

#define Malloc(type,n) (type *)malloc((n)*sizeof(type))

//#define B5M_DEBUG

ProductMatcher::ProductMatcher(const std::string& path)
:path_(path), is_open_(false), aid_manager_(NULL), analyzer_(NULL), char_analyzer_(NULL)
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
        cconfig.symbol = true;
        char_analyzer_ = new idmlib::util::IDMAnalyzer(cconfig);
        std::string cidset_path = path_+"/cid_set";
        izenelib::am::ssf::Util<>::Load(cidset_path, cid_set_);
        std::string products_path = path_+"/products";
        izenelib::am::ssf::Util<>::Load(products_path, products_);
        std::string a2p_path = path_+"/a2p";
        izenelib::am::ssf::Util<>::Load(a2p_path, a2p_);
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

    for(uint32_t i=0;i<runtime_path.size();i++)
    {
        boost::filesystem::remove_all(path+"/"+runtime_path[i]);
    }
}


bool ProductMatcher::Index(const std::string& scd_path)
{
    if(!a2p_.empty())
    {
        std::cout<<"product trained at "<<path_<<std::endl;
        return true;
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
        products_.push_back(product);
        //logger_<<"[BPD][Title]"<<stitle<<std::endl;
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
            if(value_list.size()!=2)
            {
                value_list.resize(1);
                value_list[0] = svalue;
            }
            std::vector<UString> my_value_list;
            for(std::size_t j=0;j<value_list.size();j++)
            {
                boost::algorithm::to_lower(value_list[j]);
                my_value_list.push_back(UString(value_list[j], UString::UTF_8));
            }
            my_attrib_list.push_back(std::make_pair(attrib_name, my_value_list));
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
    UString category;
    UString text;
    doc.getProperty("Category", category);
    doc.getProperty("Title", text);
    if(text.empty()) return false;

    UString udocid;
    doc.getProperty("DOCID", udocid);
    std::string sdocid;
    udocid.convertString(sdocid, UString::UTF_8);
    if(sdocid=="d50b08b1530d0fde3ceb177c4c74c931")
    {
        std::cout<<"!!!!!!!!!!!find debug"<<std::endl;
    }

    //std::string stext;
    //text.convertString(stext, UString::UTF_8);
    //std::string scategory;
    //category.convertString(scategory, UString::UTF_8);
    //std::cout<<"text : "<<stext<<std::endl;
    if(!category.empty())
    {
        std::string scategory;
        category.convertString(scategory, UString::UTF_8);
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
            product.spid = B5MHelper::Uint128ToString(pid);
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
    if(sdocid=="d50b08b1530d0fde3ceb177c4c74c931")
    {
        std::cout<<"!!!!show debug aid"<<std::endl;
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
        CAttribId caid = 0;
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
            double str_sim = StringSimilarity::Sim(text, UString(this_product.stitle, UString::UTF_8));
#ifdef B5M_DEBUG
            std::cout<<"sim with "<<pid<<","<<this_product.stitle<<" : "<<str_sim<<std::endl;
#endif
            product_candidate.push_back(std::make_pair(str_sim, this_product));
        }
    }
    if(!product_candidate.empty())
    {
        typedef izenelib::util::first_greater<std::pair<double, Product> > greater_than;
        std::sort(product_candidate.begin(), product_candidate.end(), greater_than());
        product = product_candidate[0].second;
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
    static const uint32_t max_unmatch_count = 3;
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

