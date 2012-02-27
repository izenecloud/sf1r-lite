#include "attribute_indexer.h"
#include "b5m_helper.h"
#include <common/ScdParser.h>
#include <common/ScdWriter.h>
#include <document-manager/Document.h>
#include <mining-manager/util/split_ustr.h>
#include <product-manager/product_price.h>
#include <boost/unordered_set.hpp>
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int.hpp>
#include <algorithm>
#include <cmath>
#include <idmlib/util/svm.h>
#include <idmlib/similarity/string_similarity.h>
#include <util/functional.h>
using namespace sf1r;
using namespace idmlib::sim;

#define Malloc(type,n) (type *)malloc((n)*sizeof(type))

#define B5M_DEBUG

AttributeIndexer::AttributeIndexer()
:is_open_(false), index_(NULL), id_manager_(NULL), analyzer_(NULL), char_analyzer_(NULL)
{
}

AttributeIndexer::~AttributeIndexer()
{
    if(index_!=NULL) 
    {
        index_->close();
        delete index_;
    }
    if(id_manager_!=NULL) 
    {
        id_manager_->close();
        delete id_manager_;
    }

    if(name_index_!=NULL) 
    {
        name_index_->close();
        delete name_index_;
    }
    if(name_id_manager_!=NULL) 
    {
        name_id_manager_->close();
        delete name_id_manager_;
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

bool AttributeIndexer::LoadSynonym(const std::string& file)
{
    //insert to synonym_map_
    std::ifstream ifs(file.c_str());
    std::string line;
    while( getline(ifs, line) )
    {
        boost::algorithm::trim(line);
        if(line.empty()) continue;
        if(line[0]=='#') continue;
        boost::to_lower(line);
        std::vector<std::string> words;
        boost::algorithm::split( words, line, boost::algorithm::is_any_of(",") );
        if(words.size()<2) continue;
        boost::regex r(words[0]);
        std::string formatter = " "+words[1]+" ";
        normalize_pattern_.push_back(std::make_pair(r, formatter));
        //std::string base = words[0];
        //for(std::size_t i=1;i<words.size();i++)
        //{
            //synonym_map_.insert(std::make_pair(words[i], base));
        //}
    }
    ifs.close();
    return true;
}

bool AttributeIndexer::Index(const std::string& scd_file, const std::string& knowledge_dir)
{
    std::string done_file = knowledge_dir+"/index.done";
    if(boost::filesystem::exists(done_file))
    {
        std::cout<<knowledge_dir<<" index done, ignore."<<std::endl;
        return true;
    }
    ClearKnowledge_(knowledge_dir);
    Open(knowledge_dir);
    BuildProductDocuments_(scd_file);
    if(product_list_.empty())
    {
        LOG(INFO)<<"product_list_ empty, ignore training."<<std::endl;
        return true;
    }
    WriteIdInfo_();
    std::string pa_path = knowledge_dir+"/product_list";
    izenelib::am::ssf::Util<>::Save(pa_path, product_list_);
    std::string filter_attrib_name_file = knowledge_dir+"/filter_attrib_name";
    std::ofstream fofs(filter_attrib_name_file.c_str());
    boost::unordered_set<AttribNameId>::iterator it = filter_anid_.begin();
    while(it!=filter_anid_.end())
    {
        AttribRep name_rep;
        AttribNameId anid = *it;
        name_id_manager_->getDocNameByDocId(anid, name_rep);
        std::string str;
        name_rep.convertString(str, izenelib::util::UString::UTF_8);
        fofs<<str<<std::endl;
        ++it;
    }
    fofs.close();


    //std::string fanid_path = knowledge_dir+"/filter_anid";
    //std::vector<AttribNameId> filter_anid_list;
    //boost::unordered_set<AttribNameId>::iterator it = filter_anid_.begin();
    //while(it!=filter_anid_.end())
    //{
        //filter_anid_list.push_back(*it);
        //++it;
    //}
    //izenelib::am::ssf::Util<>::Save(fanid_path, filter_anid_list);
    GenClassiferInstance();
    std::ofstream ofs(done_file.c_str());
    ofs<<"done"<<std::endl;
    ofs.close();
    return true;
}

void AttributeIndexer::ClearKnowledge_(const std::string& knowledge_dir)
{
    if(!boost::filesystem::exists(knowledge_dir)) return;
    std::vector<std::string> runtime_path;
    runtime_path.push_back("logger");
    runtime_path.push_back("attrib_id");
    runtime_path.push_back("attrib_index");
    runtime_path.push_back("attrib_name_id");
    runtime_path.push_back("attrib_name_index");
    runtime_path.push_back("product_list");
    runtime_path.push_back("filter_attrib_name");

    for(uint32_t i=0;i<runtime_path.size();i++)
    {
        boost::filesystem::remove_all(knowledge_dir+"/"+runtime_path[i]);
    }
}

bool AttributeIndexer::Open(const std::string& knowledge_dir)
{
    if(!is_open_)
    {
        knowledge_dir_ = knowledge_dir;
        boost::filesystem::create_directories(knowledge_dir);
        std::string logger_file = knowledge_dir+"/logger";
        logger_.open(logger_file.c_str(), std::ios::out | std::ios::app );
        std::string id_dir = knowledge_dir+"/attrib_id";
        boost::filesystem::create_directories(id_dir);
        id_manager_ = new AttribIDManager(id_dir+"/id");
        index_ = new AttribIndex(knowledge_dir+"/attrib_index");
        index_->open();
        std::string name_id_dir = knowledge_dir+"/attrib_name_id";
        boost::filesystem::create_directories(name_id_dir);
        name_id_manager_ = new AttribNameIDManager(name_id_dir+"/id");
        name_index_ = new AttribNameIndex(knowledge_dir+"/attrib_name_index");
        name_index_->open();
        idmlib::util::IDMAnalyzerConfig aconfig = idmlib::util::IDMAnalyzerConfig::GetCommonConfig("",cma_path_, "");
        aconfig.symbol = true;
        analyzer_ = new idmlib::util::IDMAnalyzer(aconfig);
        idmlib::util::IDMAnalyzerConfig cconfig = idmlib::util::IDMAnalyzerConfig::GetCommonConfig("","", "");
        cconfig.symbol = true;
        char_analyzer_ = new idmlib::util::IDMAnalyzer(cconfig);
        std::string pa_path = knowledge_dir+"/product_list";
        izenelib::am::ssf::Util<>::Load(pa_path, product_list_);


        //predefined knowledges
        std::string category_file = knowledge_dir+"/category";
        if(boost::filesystem::exists(category_file))
        {
            std::ifstream cifs(category_file.c_str());
            std::string line;
            if(getline(cifs, line))
            {
                boost::algorithm::trim(line);
                LOG(INFO)<<"find category regex : "<<line<<std::endl;
                SetCategoryRegex(line);
            }
            cifs.close();
        }

        uint32_t num_filter_attrib = 0;
        std::string filter_attrib_name_file = knowledge_dir+"/filter_attrib_name";
        if(boost::filesystem::exists(filter_attrib_name_file ))
        {
            std::ifstream fifs(filter_attrib_name_file.c_str());
            std::string line;
            while(getline(fifs, line))
            {
                boost::algorithm::trim(line);
                if(line.length()==0) continue;
                if(line[0]=='#') continue;
                //LOG(INFO)<<"find filter attrib name : "<<line<<std::endl;
                filter_attrib_name_.insert(line);
                AttribRep name_rep(line, izenelib::util::UString::UTF_8);
                AttribNameId name_aid;
                if(name_id_manager_->getDocIdByDocName(name_rep, name_aid, false))
                {
                    filter_anid_.insert(name_aid);
                }
                num_filter_attrib++;
            }
            fifs.close();
        }
        LOG(INFO)<<"find "<<num_filter_attrib<<" filter attrib name"<<std::endl;

        is_open_ = true;
    }
    return true;
}

void AttributeIndexer::NormalizeText_(const izenelib::util::UString& text, izenelib::util::UString& ntext)
{
    std::string str;
    text.convertString(str, izenelib::util::UString::UTF_8);
    //logger_<<"[BNO]"<<str<<std::endl;
    boost::to_lower(str);
    for(uint32_t i=0;i<normalize_pattern_.size();i++)
    {
        str = boost::regex_replace(str, normalize_pattern_[i].first, normalize_pattern_[i].second);
    }
    //logger_<<"[ANO]"<<str<<std::endl;
    ntext = izenelib::util::UString(str, izenelib::util::UString::UTF_8);
}

void AttributeIndexer::Analyze_(const izenelib::util::UString& btext, std::vector<izenelib::util::UString>& result)
{
    AnalyzeImpl_(analyzer_, btext, result);
}

void AttributeIndexer::AnalyzeChar_(const izenelib::util::UString& btext, std::vector<izenelib::util::UString>& result)
{
    AnalyzeImpl_(char_analyzer_, btext, result);
}

void AttributeIndexer::AnalyzeImpl_(idmlib::util::IDMAnalyzer* analyzer, const izenelib::util::UString& btext, std::vector<izenelib::util::UString>& result)
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
//void AttributeIndexer::AnalyzeSynonym_(const izenelib::util::UString& text, std::vector<B5MToken>& tokens)
//{
    //std::vector<izenelib::util::UString> str_list;
    //AnalyzeChar_(text, str_list);
    //typedef SynonymHandler::Flag Flag;
    //Flag pflag = SynonymHandler::RootFlag();
    //Flag flag;
    //std::size_t index = 0;
    //SynonymHandler::SYN_STATUS syn_status;
    //std::vector<B5MToken> cache_tokens;
    //while(index<str_list.size())
    //{
        //syn_status = synonym_handler_.Search(pflag, str_list[index], flag);
        //if(syn_status == SynonymHandler::SYN_NO)
        //{
            //if(!cache_tokens.empty())
            //{
                //for(uint32_t i=0;i<cache_tokens.size();i++)
                //{
                    //tokens.push_back(cache_tokens[i]);
                //}
                //cache_tokens.resize(0);
            //}
            //B5MToken t(str_list[index], B5MToken::TOKEN_C);
            //tokens.push_back(t);
            //pflag = SynonymHandler::RootFlag();
        //}
        //else if(syn_status == SynonymHandler::SYN_INTER)
        //{
            //B5MToken t(str_list[index], B5MToken::TOKEN_C);
            //cache_tokens.push_back(t);
            //pflag = flag;
        //}
        //else //SYN_YES
        //{
            ////the  ABC=>XX, ABCD=>YY is not supported.
            //izenelib::util::UString standard;
            //synonym_handler_.Get(flag, standard);
            //B5MToken t(standard, B5MToken::TOKEN_S);
            //tokens.push_back(t);
            //cache_tokens.resize(0);
            //pflag = SynonymHandler::RootFlag();
        //}
        //++index;
    //}
//}

void AttributeIndexer::NormalizeAV_(const izenelib::util::UString& av, izenelib::util::UString& nav)
{
    std::vector<izenelib::util::UString> termstr_list;
    Analyze_(av, termstr_list);

    //{
        //std::string sav;
        //av.convertString(sav, izenelib::util::UString::UTF_8);
        //logger_<<"[O]"<<sav<<std::endl;
        //for(std::size_t i=0;i<termstr_list.size();i++)
        //{
            //std::string s;
            //termstr_list[i].convertString(s, izenelib::util::UString::UTF_8);
            //logger_<<"[T]"<<s<<std::endl;
        //}
    //}
    boost::unordered_set<std::string> app;
    for(std::size_t i=0;i<termstr_list.size();i++)
    {
        std::string str;
        termstr_list[i].convertString(str, izenelib::util::UString::UTF_8);
        if(app.find(str)!=app.end()) continue;
        app.insert(str);
        nav.append( izenelib::util::UString(str, izenelib::util::UString::UTF_8) );

    }
}

bool AttributeIndexer::IsBoolAttribValue_(const izenelib::util::UString& av, bool& b)
{
    std::string sav;
    av.convertString(sav, izenelib::util::UString::UTF_8);
    if(sav=="支持" || sav=="有" || sav=="是")
    {
        b = true;
        return true;
    }
    else if(sav=="不支持" || sav=="没有" || sav=="否")
    {
        b = false;
        return true;
    }
    return false;
}

void AttributeIndexer::GetAttribIdList(const izenelib::util::UString& value, std::vector<AttribId>& id_list)
{
    izenelib::util::UString category;
    GetAttribIdList(category, value, id_list);
}

void AttributeIndexer::GetNgramAttribIdList_(const izenelib::util::UString& ngram, std::vector<AttribId>& aid_list)
{
    std::vector<izenelib::util::UString> ngram_list;
    ngram_list.push_back(ngram);
    ngram_synonym_.Get(ngram, ngram_list);
    for(std::size_t s=0;s<ngram_list.size();s++)
    {
        std::vector<AttribId> aid_list_value;
        //{
            //std::string sn;
            //ngram.convertString(sn, izenelib::util::UString::UTF_8);
            //std::cout<<"[NGRAM] "<<sn<<std::endl;
        //}
        index_->get(ngram_list[s], aid_list_value);
        for(std::size_t a=0;a<aid_list_value.size();++a)
        {
            AttribId aid = aid_list_value[a];
            if(ValidAid_(aid))
            {
                aid_list.push_back(aid);
            }
        }
    }
}

void AttributeIndexer::GetAttribIdList(const izenelib::util::UString& category, const izenelib::util::UString& value, std::vector<AttribId>& id_list)
{
    static const uint32_t n = 10;
    static const uint32_t max_ngram_len = 20;
    static const uint32_t max_unmatch_count = 1;
    std::vector<izenelib::util::UString> termstr_list;
    AnalyzeChar_(value, termstr_list);
    std::size_t index = 0;
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
            GetNgramAttribIdList_(ngram, aid_list);
            //{
                //std::string sngram;
                //ngram.convertString(sngram, izenelib::util::UString::UTF_8);
                //logger_<<"[FN]"<<sngram<<" [";
                //for(uint32_t i=0;i<aid_list.size();i++)
                //{
                    //logger_<<aid_list[i]<<",";
                //}
                //logger_<<std::endl;
            //}
            if(!aid_list.empty())
            {
                inc_len = len+1;
                attrib_ngram = ngram;
                ngram_aid_list.swap(aid_list);
                unmatch_count = 0;
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
        id_list.insert(id_list.end(), ngram_aid_list.begin(), ngram_aid_list.end());
        if(attrib_ngram.length()>0)
        {
            index += inc_len;
            //index++;
        }
        else
        {
            ++index;
        }
    }
    //for(std::size_t i=0;i<termstr_list.size();i++)
    //{
        //izenelib::util::UString ngram;
        //for(std::size_t j=0;j<n;j++)
        //{
            //std::size_t index = i+j;
            //if(index>=termstr_list.size()) break;
            //ngram.append(termstr_list[index]);
            //if(ngram.length()<2) continue;
            //if(ngram.length()>max_ngram_len) break;
            //std::vector<izenelib::util::UString> ngram_list;
            //ngram_list.push_back(ngram);
            //ngram_synonym_.Get(ngram, ngram_list);
            //for(std::size_t s=0;s<ngram_list.size();s++)
            //{
                //std::vector<AttribId> aid_list_value;
                ////{
                    ////std::string sn;
                    ////ngram.convertString(sn, izenelib::util::UString::UTF_8);
                    ////std::cout<<"[NGRAM] "<<sn<<std::endl;
                ////}
                //index_->get(ngram_list[s], aid_list_value);
                //for(std::size_t a=0;a<aid_list_value.size();++a)
                //{
                    //AttribId aid = aid_list_value[a];
                    //AttribNameId anid;
                    //name_index_->get(aid, anid);
                    //if(filter_anid_.find(anid)==filter_anid_.end())
                    //{
                        //id_list.push_back(aid);
                    //}
                //}
            //}
        //}
    //}
    std::sort(id_list.begin(), id_list.end());
    id_list.erase(std::unique(id_list.begin(), id_list.end()), id_list.end());
    //filter by category
    if(category.length()>0)
    {
        std::vector<AttribId> category_id_list;
        for(std::size_t i=0;i<id_list.size();i++)
        {
            AttribRep rep;
            if(!GetAttribRep(id_list[i], rep)) continue;
            if(rep.find(category)!=0) continue;
            category_id_list.push_back(id_list[i]);
        }
        id_list.swap(category_id_list);
    }
}

bool AttributeIndexer::GetAttribRep(const AttribId& aid, AttribRep& rep)
{
    return id_manager_->getDocNameByDocId(aid, rep);
}

bool AttributeIndexer::GetAttribNameId(const AttribId& aid, AttribNameId& name_id)
{
    return name_index_->get(aid, name_id);
}

void AttributeIndexer::GenNegativeIdMap_(std::map<std::size_t, std::vector<std::size_t> >& map)
{
    static const uint32_t negative_num = 1000;
    boost::unordered_map<std::string, std::vector<std::size_t> > category_indexlist;
    boost::unordered_map<std::string, std::vector<std::size_t> >::iterator ci_it;
    BuildCategoryMap_(category_indexlist);
    NonZeroFC fc;
    //boost::mt19937 random_gen;
    for(std::size_t i=0;i<product_list_.size();i++)
    {
        if(i%100==0)
        {
            LOG(INFO)<<"Building Negative Map "<<i<<std::endl;
        }
        std::vector<std::size_t> negative_index;
        ProductDocument& doc = product_list_[i];
        std::string scategory;
        doc.property["Category"].convertString(scategory, izenelib::util::UString::UTF_8);
        ci_it = category_indexlist.find(scategory);
        std::vector<std::size_t>& indexlist_in_category = ci_it->second;
        std::vector<std::pair<std::size_t, uint32_t> > feature_num_vec;
        for(std::size_t p=0;p<indexlist_in_category.size();p++)
        {
            std::size_t index = indexlist_in_category[p];
            ProductDocument& cdoc = product_list_[index];
            if( index==i ) continue;
            if( doc.tag_aid_list== cdoc.tag_aid_list ) 
            {
                std::cout<<"[duplicate] ("<<i<<","<<index<<") "<<doc.property["DOCID"]<<","<<cdoc.property["DOCID"]<<std::endl;
                continue;
            }
            std::vector<std::pair<AttribNameId, double> > n_feature_vec;
            FeatureStatus fs;
            GetFeatureVector_(doc.aid_list, cdoc.tag_aid_list, n_feature_vec, fs); 
            uint32_t nznum = fs.pnum+fs.nnum;
            if(nznum==0)
            {
                continue;
            }
            //if(fs.nnum==0) continue;
            feature_num_vec.push_back(std::make_pair(index, nznum));
        }
        typedef izenelib::util::second_greater<std::pair<std::size_t, uint32_t> > greater_than;
        std::sort(feature_num_vec.begin(), feature_num_vec.end(), greater_than());
        for(std::size_t p=0;p<feature_num_vec.size();p++)
        {
            if(feature_num_vec[p].second<2) break;
            if(feature_num_vec[p].first==i) continue;
            negative_index.push_back(feature_num_vec[p].first);
            if(negative_index.size()>=negative_num) break;
        }
        std::sort(negative_index.begin(), negative_index.end());
        //if(!indexlist_in_category.empty())
        //{
            //std::vector<std::size_t> random_indexlist(indexlist_in_category.begin(), indexlist_in_category.end());
            //std::random_shuffle(random_indexlist.begin(), random_indexlist.end());
            //std::size_t p = 0;
            //while(true)
            //{
                //if(p>=random_indexlist.size()) break;
                //if(negative_index.size()>=negative_num) break;
                //std::size_t random_index = random_indexlist[p++];
                //if(random_index==i) continue;
                //negative_index.push_back(random_index);
            //}
        //}
        map.insert(std::make_pair(i, negative_index));
    }
}

void AttributeIndexer::GenClassiferInstance()
{
    //attribute id in libsvm file is the attribute name id.
    std::string output = knowledge_dir_+"/instances";
    std::ofstream ofs(output.c_str());
    std::map<std::size_t, std::vector<std::size_t> > negative_map;
    GenNegativeIdMap_(negative_map);
    PositiveFC pfc;
    NonNegativeFC nnfc;
    NonZeroFC nzfc;
    for(std::size_t p=0;p<product_list_.size();++p)
    {
        if(p%100==0)
        {
            LOG(INFO)<<"generating instance "<<p<<std::endl;
        }
        ProductDocument& product_doc = product_list_[p];
        std::vector<AttribId>& aid_list = product_doc.aid_list;
        std::vector<AttribId>& tag_aid_list = product_doc.tag_aid_list;
        std::vector<std::pair<AttribNameId, double> > feature_vec;
        FeatureStatus fs;
        GetFeatureVector_(aid_list, tag_aid_list, feature_vec, fs);
        if(fs.nnum>0)
        {
            RemoveNegative_(feature_vec);
        }
#ifdef B5M_DEBUG
        {
            std::string stitle;
            product_doc.property["Title"].convertString(stitle, izenelib::util::UString::UTF_8);
            logger_<<"[OT]"<<stitle<<std::endl;
            //logger_<<"[AID]"<<std::endl;
            //for(std::size_t i=0;i<aid_list.size();i++)
            //{
                //AttribRep rep;
                //GetAttribRep(aid_list[i], rep);
                //std::string srep;
                //rep.convertString(srep, izenelib::util::UString::UTF_8);
                //logger_<<aid_list[i]<<","<<srep<<std::endl;
            //}
            //logger_<<"[TAID]"<<std::endl;
            //for(std::size_t i=0;i<tag_aid_list.size();i++)
            //{
                //AttribRep rep;
                //GetAttribRep(tag_aid_list[i], rep);
                //std::string srep;
                //rep.convertString(srep, izenelib::util::UString::UTF_8);
                //logger_<<tag_aid_list[i]<<","<<srep<<std::endl;
            //}
            logger_<<"[F]"<<std::endl;
            for(std::size_t i=0;i<feature_vec.size();i++)
            {
                logger_<<feature_vec[i].first<<":"<<feature_vec[i].second<<std::endl;
            }
        }
#endif
        if(fs.pnum>1)
        {
            ofs<<"+1";
            for(std::size_t i=0;i<feature_vec.size();i++)
            {
                ofs<<" "<<feature_vec[i].first<<":"<<feature_vec[i].second;
            }
            ofs<<std::endl;
        }
        std::vector<std::size_t>& negative_list = negative_map[p];
        for(std::size_t i=0;i<negative_list.size();++i)
        {
            ProductDocument& negative_doc = product_list_[negative_list[i]];
            std::string stitle;
            negative_doc.property["Title"].convertString(stitle, izenelib::util::UString::UTF_8);
            std::vector<AttribId>& n_tag_aid_list = negative_doc.tag_aid_list;
            std::vector<std::pair<AttribNameId, double> > n_feature_vec;
            FeatureStatus fs;
            GetFeatureVector_(aid_list, n_tag_aid_list, n_feature_vec, fs);
            //GetFeatureVector_(aid_list, n_tag_aid_list, n_feature_vec, nzfc);
            //reset n_feature_vec if all positive
            //if(n_feature_vec.size()>1)
            //{
                //bool all_positive = true;
                //for(uint32_t f=0;f<n_feature_vec.size();f++)
                //{
                    //if(n_feature_vec[f].second<=0.0)
                    //{
                        //all_positive = false;
                        //break;
                    //}
                //}
                //if(all_positive)
                //{
                    //n_feature_vec.resize(0);
                    //GetFeatureVector_(tag_aid_list, n_tag_aid_list, n_feature_vec, nzfc);
                //}

            //}
            if(fs.pnum+fs.nnum>1)
            {
                ofs<<"-1";
                for(std::size_t i=0;i<n_feature_vec.size();i++)
                {
                    ofs<<" "<<n_feature_vec[i].first<<":"<<n_feature_vec[i].second;
                }
                ofs<<std::endl;
            }
#ifdef B5M_DEBUG
            {
                logger_<<"[NT]"<<stitle<<std::endl;
                if(n_feature_vec.size()>1)
                {
                    logger_<<"-1";
                    for(std::size_t i=0;i<n_feature_vec.size();i++)
                    {
                        logger_<<" "<<n_feature_vec[i].first<<":"<<n_feature_vec[i].second;
                    }
                    logger_<<std::endl;
                }
                
            }
#endif
        }

    }
    

    ofs.close();
}

bool AttributeIndexer::TrainSVM()
{
    LOG(INFO)<<"start to train svm"<<std::endl;
    struct svm_parameter param;
    struct svm_problem problem;
    struct svm_model* model;
    struct svm_node* x_space;
	param.svm_type = C_SVC;
	param.kernel_type = RBF;
	param.degree = 3;
	param.gamma = 0.0;	// 1/num_features
	param.coef0 = 0;
	param.nu = 0.5;
	param.cache_size = 100;
	param.C = 32;
	param.eps = 1e-3;
	param.p = 0.1;
	param.shrinking = 1;
	param.probability = 0;
	param.nr_weight = 0;
	param.weight_label = NULL;
	param.weight = NULL;
    std::string instance_file = knowledge_dir_+"/instances";
    std::string model_file = knowledge_dir_+"/model";
    if(!boost::filesystem::exists(instance_file))
    {
        std::cout<<"instances file not exists"<<std::endl;
        return false;
    }
    std::ifstream ifs(instance_file.c_str());
    std::string line;
    problem.l = 0;
    uint32_t elements = 0;
    while(getline(ifs, line))
    {
        boost::algorithm::trim(line);
        std::vector<std::string> tokens;
        boost::algorithm::split( tokens, line, boost::algorithm::is_any_of(" \t") );
        elements += tokens.size(); // tokens.size-1 +1
        problem.l++;
    }
    ifs.close();
    ifs.open(instance_file.c_str());
    problem.y = Malloc(double, problem.l);
    problem.x = Malloc(struct svm_node*, problem.l);
    x_space = Malloc(struct svm_node, elements);
    int n = 0;
    int max_index = 0;
    for(int i=0;i<problem.l;i++)
    {
        problem.x[i] = &x_space[n];
        std::string line;
        getline(ifs, line);
        boost::algorithm::trim(line);
        std::vector<std::string> tokens;
        boost::algorithm::split( tokens, line, boost::algorithm::is_any_of(" \t") );
        if(tokens.size()<1)
        {
            std::cout<<"invalid line : "<<i+1<<std::endl;
            return false;
        }
        problem.y[i] = boost::lexical_cast<double>(tokens[0]);
        for(uint32_t j=1;j<tokens.size();j++)
        {
            std::vector<std::string> node_pair;
            boost::algorithm::split(node_pair, tokens[j], boost::algorithm::is_any_of(":"));
            if(node_pair.size()!=2)
            {
                std::cout<<"invalid line : "<<i+1<<std::endl;
                return false;
                
            }
            x_space[n].index = boost::lexical_cast<int>(node_pair[0]);
            x_space[n].value = boost::lexical_cast<double>(node_pair[1]);
            if(x_space[n].index>max_index)
            {
                max_index = x_space[n].index;
            }
            ++n;
            
        }
        x_space[n++].index = -1;
    }
    if(param.gamma == 0.0 && max_index>0)
    {
        param.gamma = 1.0/max_index;
    }
    ifs.close();
    model = svm_train(&problem, &param);
    if(svm_save_model(model_file.c_str(), model))
    {
        std::cout<<"can not save svm model"<<std::endl;
        return false;
    }
    svm_free_and_destroy_model(&model);
    svm_destroy_param(&param);
    free(problem.y);
    free(problem.x);
    free(x_space);
    LOG(INFO)<<"finish training svm"<<std::endl;
    return true;
}

void AttributeIndexer::ProductMatchingSVM(const std::string& scd_path)
{
    std::string done_file = knowledge_dir_+"/match.done";
    if(boost::filesystem::exists(done_file))
    {
        return;
    }
    if(product_list_.empty())
    {
        std::cout<<"product_list_ empty, ignore matching"<<std::endl;
        return;
    }
    std::string match_file = knowledge_dir_+"/match";
    struct svm_model* model;
    std::string model_file = knowledge_dir_+"/model";
    if( (model=svm_load_model(model_file.c_str()))==0 )
    {
        std::cout<<"load model failed"<<std::endl;
        return;
    }
    std::cout<<model_file<<" loaded!"<<std::endl;
    std::ofstream match_ofs(match_file.c_str());
    boost::unordered_map<std::string, std::vector<std::size_t> > category_indexlist;
    boost::unordered_map<std::string, std::vector<std::size_t> >::iterator ci_it;
    BuildCategoryMap_(category_indexlist);
    //std::vector<StringSimilarity*> ss_list(product_list_.size());
    //for(std::size_t i=0;i<ss_list.size();i++)
    //{
        //ss_list[i] = new StringSimilarity(product_list_[i].property["Title"]);
    //}
    std::string scd_file = scd_path;
    bool inner_scd = false;
    std::string a_scd = knowledge_dir_+"/A.SCD";
    uint32_t counting = 100000;
    if(boost::filesystem::exists(a_scd))
    {
        scd_file = a_scd;
        inner_scd = true;
        counting = 200;
        std::cout<<"USE A.SCD FOR MATCHING"<<std::endl;
    }
    static const double price_ratio = 2.2;
    static const double invert_price_ratio = 1.0/price_ratio;
    ScdParser parser(izenelib::util::UString::UTF_8);
    parser.load(scd_file);
    NonZeroFC nzfc;
    uint32_t n=0;
    uint32_t num_for_match = 0;
    boost::unordered_map<std::string, std::vector<std::string> > p2o_map;
    for( ScdParser::iterator doc_iter = parser.begin(B5MHelper::B5M_PROPERTY_LIST);
      doc_iter!= parser.end(); ++doc_iter, ++n)
    {
        if(n%counting==0)
        {
            LOG(INFO)<<"Processing "<<n<<" docs"<<std::endl;
        }
        ProductDocument odoc;
        izenelib::util::UString oid;
        izenelib::util::UString title;
        izenelib::util::UString category;
        izenelib::util::UString url;
        izenelib::util::UString attrib_ustr;
        SCDDoc& doc = *(*doc_iter);
        std::vector<std::pair<izenelib::util::UString, izenelib::util::UString> >::iterator p;
        for(p=doc.begin(); p!=doc.end(); ++p)
        {
            std::string property_name;
            p->first.convertString(property_name, izenelib::util::UString::UTF_8);
            odoc.property[property_name] = p->second;
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
            else if(property_name=="Url")
            {
                url = p->second;
            }
        }
        if( category.length()==0 || title.length()==0) continue;
        std::string soid;
        oid.convertString(soid, izenelib::util::UString::UTF_8);
        std::string stitle;
        title.convertString(stitle, izenelib::util::UString::UTF_8);
        std::string scategory;
        category.convertString(scategory, izenelib::util::UString::UTF_8);
        std::string surl;
        url.convertString(surl, izenelib::util::UString::UTF_8);
        if(!inner_scd)
        {
            if( !match_param_.MatchCategory(scategory) )
            {
                continue;
            }
        }
        ci_it = category_indexlist.find(scategory);
        if( ci_it==category_indexlist.end()) continue;
        ProductPrice price;
        price.Parse(odoc.property["Price"]);
        if(!price.Valid()) continue;
        
        std::vector<std::size_t>& pindexlist = ci_it->second;
        std::vector<AttribId> aid_list;
        GetAttribIdList(category, title, aid_list);
        std::vector<std::pair<std::size_t, double> > match_list;
        for(std::size_t p=0;p<pindexlist.size();++p)
        {
            std::size_t pindex = pindexlist[p];
            ProductDocument& product_doc = product_list_[pindex];
            ProductPrice pprice;
            pprice.Parse(product_doc.property["Price"]);
            std::vector<std::pair<AttribNameId, double> > feature_vec;
            FeatureStatus fs;
            //GetFeatureVector_(aid_list, product_doc.tag_aid_list, feature_vec, nzfc);
            GetFeatureVector_(aid_list, product_doc.tag_aid_list, feature_vec, fs);
            //if(fs.pnum+fs.nnum<2) continue;
            struct svm_node *x = (struct svm_node *) malloc((feature_vec.size()+1)*sizeof(struct svm_node));
            for(uint32_t i=0;i<feature_vec.size();i++)
            {
                x[i].index = (int)feature_vec[i].first;
                x[i].value = feature_vec[i].second;
            }
            x[feature_vec.size()].index = -1;
            double predict_label = svm_predict(model,x);
            free(x);
#ifdef B5M_DEBUG
            {
                logger_<<"[MATCH]"<<std::endl;
                std::string sptitle;
                product_doc.property["Title"].convertString(sptitle, izenelib::util::UString::UTF_8);
                //logger_<<scategory<<std::endl;
                logger_<<"[MO]"<<stitle<<std::endl;
                logger_<<surl<<std::endl;
                for(uint32_t i=0;i<aid_list.size();++i)
                {
                    logger_<<aid_list[i]<<",";
                }
                logger_<<std::endl;
                logger_<<"[MP]"<<sptitle<<std::endl;
                for(uint32_t i=0;i<product_doc.tag_aid_list.size();++i)
                {
                    logger_<<product_doc.tag_aid_list[i]<<",";
                }
                logger_<<std::endl;
                logger_<<"feature size: "<<feature_vec.size()<<std::endl;
                for(uint32_t i=0;i<feature_vec.size();i++)
                {
                    logger_<<feature_vec[i].first<<":"<<feature_vec[i].second<<",";
                }
                logger_<<std::endl;
                logger_<<(int)predict_label<<std::endl;
            }
#endif
            if(predict_label>0)
            {
                if(fs.pnum+fs.nnum<2) continue;
                double om, pm;
                if(!price.GetMid(om) || !pprice.GetMid(pm)) continue;
                if(om<=0.0 || pm<=0.0) continue;
                double ratio = om/pm;
                if(ratio<invert_price_ratio || ratio>price_ratio) continue;
                double str_sim = StringSimilarity::Sim(title, product_doc.property["Title"]);
                //double str_sim = ss_list[pindex]->Sim(title);
                match_list.push_back(std::make_pair(pindex, str_sim));

                //{
                    //izenelib::util::UString pid = product_doc.property["DOCID"];
                    //std::string spid, sptitle;
                    //pid.convertString(spid, izenelib::util::UString::UTF_8);
                    //product_doc.property["Title"].convertString(sptitle, izenelib::util::UString::UTF_8);
                    //std::cout<<"[STRSIM]["<<stitle<<"]\t["<<sptitle<<"] : "<<str_sim<<std::endl;
                //}
            }
        }
        if(!match_list.empty())
        {
            typedef izenelib::util::second_greater<std::pair<std::size_t, double> > greater_than;
            std::stable_sort(match_list.begin(), match_list.end(), greater_than());
            std::size_t match_pindex = match_list[0].first;
            ProductDocument& match_pdoc = product_list_[match_pindex];
            izenelib::util::UString pid = match_pdoc.property["DOCID"];
            std::string spid, sptitle;
            pid.convertString(spid, izenelib::util::UString::UTF_8);
            match_pdoc.property["Title"].convertString(sptitle, izenelib::util::UString::UTF_8);
            match_ofs<<soid<<","<<spid<<","<<stitle<<"\t["<<sptitle<<"]"<<std::endl;
        }
        num_for_match++;
    } 
    delete model;
    //for(std::size_t i=0;i<ss_list.size();i++)
    //{
        //delete ss_list[i];
    //}
    match_ofs.close();
    std::ofstream ofs(done_file.c_str());
    ofs<<product_list_.size()<<","<<num_for_match<<std::endl;
    ofs.close();
}

void AttributeIndexer::ProductMatchingLR(const std::string& scd_file)
{
    std::string model_file = knowledge_dir_+"/lrmodel";
    if(!boost::filesystem::exists(model_file))
    {
        std::cout<<"lrmodel not exist"<<std::endl;
        return;
    }
    std::vector<double> weights(1, 0.0);
    bool bweight = false;
    {
        std::ifstream ifs(model_file.c_str());
        std::string line;
        while(getline(ifs, line))
        {
            boost::algorithm::trim(line);
            if(bweight)
            {
                double weight = boost::lexical_cast<double>(line);
                weights.push_back(weight);
            }
            else if(line=="w")
            {
                bweight = true;
            }
        }
        ifs.close();
    }
    boost::unordered_map<std::string, std::vector<std::size_t> > category_indexlist;
    boost::unordered_map<std::string, std::vector<std::size_t> >::iterator ci_it;
    BuildCategoryMap_(category_indexlist);
    ScdParser parser(izenelib::util::UString::UTF_8);
    parser.load(scd_file);
    NonZeroFC nzfc;
    uint32_t n=0;
    for( ScdParser::iterator doc_iter = parser.begin(B5MHelper::B5M_PROPERTY_LIST);
      doc_iter!= parser.end(); ++doc_iter, ++n)
    {
        ProductDocument product_doc;
        izenelib::util::UString oid;
        izenelib::util::UString title;
        izenelib::util::UString category;
        izenelib::util::UString attrib_ustr;
        SCDDoc& doc = *(*doc_iter);
        std::vector<std::pair<izenelib::util::UString, izenelib::util::UString> >::iterator p;
        for(p=doc.begin(); p!=doc.end(); ++p)
        {
            std::string property_name;
            p->first.convertString(property_name, izenelib::util::UString::UTF_8);
            product_doc.property[property_name] = p->second;
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
            //{
                //if(property_name=="Url")
                //{
                    //std::cout<<p->second<<std::endl;
                //}
            //}
        }
        if( category.length()==0 || title.length()==0) continue;
        std::string scategory;
        category.convertString(scategory, izenelib::util::UString::UTF_8);
        ci_it = category_indexlist.find(scategory);
        if( ci_it==category_indexlist.end()) continue;
        std::vector<std::size_t>& pindexlist = ci_it->second;
        std::vector<AttribId> aid_list;
        GetAttribIdList(category, title, aid_list);
        for(std::size_t p=0;p<pindexlist.size();++p)
        {
            ProductDocument& product_doc = product_list_[pindexlist[p]];
            std::vector<std::pair<AttribNameId, double> > feature_vec;
            FeatureStatus fs;
            GetFeatureVector_(aid_list, product_doc.tag_aid_list, feature_vec, fs);
            if(feature_vec.size()<2) continue;
            {
                std::cout<<"[PR]"<<std::endl;
                std::string stitle;
                std::string sptitle;
                title.convertString(stitle, izenelib::util::UString::UTF_8);
                product_doc.property["Title"].convertString(sptitle, izenelib::util::UString::UTF_8);
                std::cout<<scategory<<std::endl;
                std::cout<<stitle<<std::endl;
                for(uint32_t i=0;i<aid_list.size();++i)
                {
                    std::cout<<aid_list[i]<<",";
                }
                std::cout<<std::endl;
                std::cout<<sptitle<<std::endl;
                for(uint32_t i=0;i<product_doc.tag_aid_list.size();++i)
                {
                    std::cout<<product_doc.tag_aid_list[i]<<",";
                }
                std::cout<<std::endl;
            }
            double z = 0;
            for(uint32_t i=0;i<feature_vec.size();i++)
            {
                AttribNameId anid = feature_vec[i].first;
                std::cout<<anid<<":"<<feature_vec[i].second<<std::endl;
                if(anid>=weights.size()) continue;
                z += weights[anid]*feature_vec[i].second;
            }
            z = z * -1;
            double predict_value = 1.0/(1.0+std::exp(z));
            int predict_label = predict_value>0.5?1:0;
            std::cout<<predict_label<<"#"<<predict_value<<std::endl;
        }
    } 
}

bool AttributeIndexer::SplitScd(const std::string& scd_file)
{
    std::string a_scd = knowledge_dir_+"/A.SCD";
    if(boost::filesystem::exists(a_scd))
    {
        std::cout<<a_scd<<" exists, ignore"<<std::endl;
        return true;
    }
    ScdParser parser(izenelib::util::UString::UTF_8);
    parser.load(scd_file);
    uint32_t n=0;
    ScdWriter writer(knowledge_dir_, INSERT_SCD);
    writer.SetFileName("A.SCD");
    for( ScdParser::iterator doc_iter = parser.begin(B5MHelper::B5M_PROPERTY_LIST);
      doc_iter!= parser.end(); ++doc_iter, ++n)
    {
        if(n%10000==0)
        {
            LOG(INFO)<<"Find Documents "<<n<<std::endl;
        }
        Document doc;
        SCDDoc& scddoc = *(*doc_iter);
        std::vector<std::pair<izenelib::util::UString, izenelib::util::UString> >::iterator p;
        for(p=scddoc.begin(); p!=scddoc.end(); ++p)
        {
            std::string property_name;
            p->first.convertString(property_name, izenelib::util::UString::UTF_8);
            doc.property(property_name) = p->second;
        }
        std::string scategory;
        doc.property("Category").get<UString>().convertString(scategory, UString::UTF_8);
        if( !match_param_.MatchCategory(scategory) )
        {
            continue;
        }
        writer.Append(doc);
    }
    writer.Close();
    return true;
}

void AttributeIndexer::GetAttribNameMap_(const std::vector<AttribId>& aid_list, boost::unordered_map<AttribNameId, std::vector<AttribId> >& map)
{
    boost::unordered_map<AttribNameId, std::vector<AttribId> >::iterator map_it;
    for(std::size_t i=0;i<aid_list.size();i++)
    {
        AttribNameId nameid;
        GetAttribNameId(aid_list[i], nameid);
        map_it = map.find(nameid);
        if(map_it==map.end())
        {
            map.insert(std::make_pair(nameid, std::vector<AttribId>(1, aid_list[i])));
        }
        else
        {
            map_it->second.push_back(aid_list[i]);
        }
    }

}

void AttributeIndexer::GetFeatureVector_(const std::vector<AttribId>& o, const std::vector<AttribId>& p, FeatureType& feature_vec, FeatureStatus& fs)
{
    boost::unordered_map<AttribNameId, std::vector<AttribId> > oname_aid_map;
    boost::unordered_map<AttribNameId, std::vector<AttribId> > pname_aid_map;
    GetAttribNameMap_(o, oname_aid_map);
    GetAttribNameMap_(p, pname_aid_map);
    boost::unordered_map<AttribNameId, std::vector<AttribId> >::iterator pit = pname_aid_map.begin();
    for(;pit!=pname_aid_map.end();++pit)
    {
        AttribNameId nameid = pit->first;
        std::vector<AttribId>& p_aid_list = pit->second;
        boost::unordered_map<AttribNameId, std::vector<AttribId> >::iterator oit = oname_aid_map.find(nameid);
        if(oit==oname_aid_map.end())
        {
            feature_vec.push_back(std::make_pair(nameid, 0.0));
            fs.znum++;
            continue;
        }
        std::vector<AttribId>& o_aid_list = oit->second;
        bool find = false;
        for(std::size_t i=0;i<o_aid_list.size();i++)
        {
            bool f = ( std::find(p_aid_list.begin(), p_aid_list.end(), o_aid_list[i])!=p_aid_list.end() );
            if(f)
            {
                find = true;
                break;
            }
        }
        double v = -1.0;
        if(find)
        {
            v = 1.0;
            fs.pnum++;
        }
        else
        {
            fs.nnum++;
        }
        feature_vec.push_back(std::make_pair(nameid, v));

    }
    std::sort(feature_vec.begin(), feature_vec.end());

}

void AttributeIndexer::BuildProductDocuments_(const std::string& scd_path)
{
    std::string scd_file = scd_path;
    bool inner_scd = false;
    std::string t_scd = knowledge_dir_+"/T.SCD";
    uint32_t counting = 10000;
    if(boost::filesystem::exists(t_scd))
    {
        scd_file = t_scd;
        inner_scd = true;
        counting = 200;
        std::cout<<"USE T.SCD FOR TRAINING"<<std::endl;
    }
    else
    {
        //force use T.SCD
        return;
    }
    ScdParser parser(izenelib::util::UString::UTF_8);
    parser.load(scd_file);
    uint32_t n=0;
    for( ScdParser::iterator doc_iter = parser.begin(B5MHelper::B5M_PROPERTY_LIST);
      doc_iter!= parser.end(); ++doc_iter, ++n)
    {
        if(n%counting==0)
        {
            LOG(INFO)<<"Find Product Documents "<<n<<std::endl;
        }
        ProductDocument product_doc;
        izenelib::util::UString oid;
        izenelib::util::UString title;
        izenelib::util::UString category;
        izenelib::util::UString attrib_ustr;
        SCDDoc& doc = *(*doc_iter);
        std::vector<std::pair<izenelib::util::UString, izenelib::util::UString> >::iterator p;
        for(p=doc.begin(); p!=doc.end(); ++p)
        {
            std::string property_name;
            p->first.convertString(property_name, izenelib::util::UString::UTF_8);
            product_doc.property[property_name] = p->second;
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
            //{
                //if(property_name=="Url")
                //{
                    //std::cout<<p->second<<std::endl;
                //}
            //}
        }
        if(category.length()==0 || attrib_ustr.length()==0 || title.length()==0)
        {
            continue;
        }
        std::string scategory;
        category.convertString(scategory, izenelib::util::UString::UTF_8);
        if(!inner_scd)
        {
            if( !match_param_.MatchCategory(scategory) )
            {
                continue;
            }
        }
        std::string stitle;
        title.convertString(stitle, izenelib::util::UString::UTF_8);
        //logger_<<"[BPD][Title]"<<stitle<<std::endl;
        std::vector<AttrPair> attrib_list;
        split_attr_pair(attrib_ustr, attrib_list);
        for(std::size_t i=0;i<attrib_list.size();i++)
        {
            const std::vector<izenelib::util::UString>& attrib_value_list = attrib_list[i].second;
            if(attrib_value_list.size()!=1) continue; //ignore empty value attrib and multi value attribs
            const izenelib::util::UString& attrib_value = attrib_value_list[0];
            const izenelib::util::UString& attrib_name = attrib_list[i].first;
            if(attrib_value.length()==0 || attrib_value.length()>30) continue;
            izenelib::util::UString nav;
            NormalizeAV_(attrib_value, nav);
            //post process nav
            bool b_value = false;
            if(IsBoolAttribValue_(nav, b_value))
            {
                //if(b_value)
                //{
                    //nav = attrib_name;
                //}
                //else
                //{
                    //nav.clear();
                //}
                //disable bool attrib now
                nav.clear();
            }
            if(nav.length()==0) continue;
            AttribRep rep = GetAttribRep(category, attrib_name, nav); 
            AttribId aid;
            id_manager_->getDocIdByDocName(rep, aid);
            AttribRep name_rep = GetAttribRep(category, attrib_name);
            AttribNameId name_aid;
            name_id_manager_->getDocIdByDocName(name_rep, name_aid);
            name_index_->insert(aid, name_aid);
            std::string sname_rep;
            name_rep.convertString(sname_rep, izenelib::util::UString::UTF_8);
            if(filter_attrib_name_.find(sname_rep)!=filter_attrib_name_.end())
            {
                filter_anid_.insert(name_aid);
            }
            if(ValidAnid_(name_aid))
            {
                product_doc.tag_aid_list.push_back(aid);
            }
            std::vector<AttribId> aid_list;
            index_->get(nav, aid_list);
            bool aid_dd = false;
            for(std::size_t j=0;j<aid_list.size();j++)
            {
                if(aid==aid_list[j])
                {
                    aid_dd = true;
                    break;
                }
            }
            if(!aid_dd)
            {
                aid_list.push_back(aid);
                index_->update(nav, aid_list);
            }
            //{
                //std::string srep;
                //rep.convertString(srep, izenelib::util::UString::UTF_8);
                //std::string snav;
                //nav.convertString(snav, izenelib::util::UString::UTF_8);
                //logger_<<srep<<" - "<<snav<<" : "<<aid<<","<<name_aid<<std::endl;
            //}

        }
        std::sort(product_doc.tag_aid_list.begin(), product_doc.tag_aid_list.end()); 
        product_list_.push_back(product_doc);
    }
    LOG(INFO)<<"Generated "<<product_list_.size()<<" docs"<<std::endl;
    //already got tag_aid_list, now build others
    std::vector<double> anid_weight(name_id_manager_->getMaxDocId()+1, 0.0);
    for(std::size_t i=0;i<product_list_.size();i++)
    {
        if(i%100==0)
        {
            LOG(INFO)<<"Building aid_list for "<<i<<std::endl;
        }
        ProductDocument& doc = product_list_[i];
        izenelib::util::UString& category = doc.property["Category"];
        izenelib::util::UString& title = doc.property["Title"];
        GetAttribIdList(category, title, doc.aid_list);

        FeatureType feature_vec;
        FeatureStatus fs;
        GetFeatureVector_(doc.aid_list, doc.tag_aid_list, feature_vec, fs);
        for(uint32_t f=0;f<feature_vec.size();f++)
        {
            AttribNameId anid = feature_vec[f].first;
            double value = feature_vec[f].second;
            anid_weight[anid] += value;
        }

    }

    double weight_threshold = 0.05*product_list_.size();

    for(AttribNameId anid=1;anid<anid_weight.size();anid++)
    {
        double weight = anid_weight[anid];
        if(weight<=weight_threshold)
        {
            filter_anid_.insert(anid);
        }
        izenelib::util::UString name;
        name_id_manager_->getDocNameByDocId(anid, name);
        std::string str;
        name.convertString(str, izenelib::util::UString::UTF_8);
        std::cout<<"[W]"<<str<<"\t"<<weight<<std::endl;
    }

    for(std::size_t i=0;i<product_list_.size();i++)
    {
        if(i%100==0)
        {
            LOG(INFO)<<"Do anid filtering for "<<i<<std::endl;
        }
        ProductDocument& doc = product_list_[i];
        std::vector<AttribId> vaid_list;
        for(std::size_t j=0;j<doc.aid_list.size();j++)
        {
            if(ValidAid_(doc.aid_list[j]))
            {
                vaid_list.push_back(doc.aid_list[j]);
            }
        }
        doc.aid_list.swap(vaid_list);

        std::vector<AttribId> vtaid_list;
        for(std::size_t j=0;j<doc.tag_aid_list.size();j++)
        {
            if(ValidAid_(doc.tag_aid_list[j]))
            {
                vtaid_list.push_back(doc.tag_aid_list[j]);
            }
        }
        doc.tag_aid_list.swap(vtaid_list);
    }

    //do de-duplicate
    std::vector<ProductDocument> product_list;
    for(std::size_t i=0;i<product_list_.size();i++)
    {
        bool dd = false;
        for(std::size_t j=0;j<product_list.size();j++)
        {
            if(product_list_[i].tag_aid_list == product_list[j].tag_aid_list)
            {
                dd = true;
                UString ititle = product_list_[i].property["Title"];
                UString jtitle = product_list[j].property["Title"];
                if(ititle.length()>0 && ititle.length()<jtitle.length())
                {
                    product_list[j].property["Title"] = ititle;
                }
                break;
            }
        }
        if(!dd)
        {
            product_list.push_back(product_list_[i]);
        }
    }
    product_list_.swap(product_list);
    LOG(INFO)<<"After DD "<<product_list_.size()<<" docs"<<std::endl;

    for(std::size_t i=0;i<product_list_.size();i++)
    {
        ProductDocument& doc = product_list_[i];
        {
            std::string stitle;
            doc.property["Title"].convertString(stitle, izenelib::util::UString::UTF_8);
            logger_<<"[PD]"<<stitle<<std::endl;
            logger_<<"[PDT]";
            for(uint32_t a=0;a<doc.tag_aid_list.size();a++)
            {
                AttribId aid = doc.tag_aid_list[a];
                izenelib::util::UString text;
                id_manager_->getDocNameByDocId(aid, text);
                std::string str;
                text.convertString(str, izenelib::util::UString::UTF_8);
                logger_<<"("<<aid<<","<<str<<")";
            }
            logger_<<std::endl;
            logger_<<"[PDA]";
            for(uint32_t a=0;a<doc.aid_list.size();a++)
            {
                AttribId aid = doc.aid_list[a];
                izenelib::util::UString text;
                id_manager_->getDocNameByDocId(aid, text);
                std::string str;
                text.convertString(str, izenelib::util::UString::UTF_8);
                logger_<<"("<<aid<<","<<str<<")";
            }
            logger_<<std::endl;
        }
    }
}


void AttributeIndexer::BuildCategoryMap_(boost::unordered_map<std::string, std::vector<std::size_t> >& map)
{
    boost::unordered_map<std::string, std::vector<std::size_t> >::iterator ci_it;
    for(std::size_t i=0;i<product_list_.size();i++)
    {
        ProductDocument& doc = product_list_[i];
        std::string scategory;
        doc.property["Category"].convertString(scategory, izenelib::util::UString::UTF_8);
        ci_it = map.find(scategory);
        if(ci_it == map.end())
        {
            map.insert(std::make_pair(scategory, std::vector<std::size_t>(1, i)));
        }
        else
        {
            ci_it->second.push_back(i);
        }
    }

}

void AttributeIndexer::WriteIdInfo_()
{
    {
        std::string output = knowledge_dir_+"/aid.txt";
        std::ofstream ofs(output.c_str());
        AttribId aid = 1;
        while(true)
        {
            izenelib::util::UString text;
            if(!id_manager_->getDocNameByDocId(aid, text)) break;
            AttribId anid;
            name_index_->get(aid, anid);
            std::string str;
            text.convertString(str, izenelib::util::UString::UTF_8);
            ofs<<aid<<"\t"<<str<<"\t"<<anid<<std::endl;
            ++aid;
        }
        ofs.close();
    }
    {
        std::string output = knowledge_dir_+"/anid.txt";
        std::ofstream ofs(output.c_str());
        AttribNameId anid = 1;
        while(true)
        {
            izenelib::util::UString text;
            if(!name_id_manager_->getDocNameByDocId(anid, text)) break;
            std::string str;
            text.convertString(str, izenelib::util::UString::UTF_8);
            ofs<<anid<<"\t"<<str<<std::endl;
            ++anid;
        }
        ofs.close();
    }
}

bool AttributeIndexer::ValidAid_(const AttribId& aid)
{
    AttribNameId anid;
    if(!name_index_->get(aid, anid)) return false;
    if(filter_anid_.find(anid)==filter_anid_.end())
    {
        return true;
    }
    return false;
}


bool AttributeIndexer::ValidAnid_(const AttribNameId& anid)
{
    if(filter_anid_.find(anid)==filter_anid_.end())
    {
        return true;
    }
    return false;
}

