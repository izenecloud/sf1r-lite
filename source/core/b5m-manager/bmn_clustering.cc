#include "bmn_clustering.h"
#include "b5m_helper.h"
#include "offer_db.h"
using namespace sf1r::b5m;
//#define BMN_DEBUG
BmnClustering::BmnClustering(const std::string& path)
: path_(path), odb_(NULL), container_(NULL), model_regex_("[a-zA-Z\\d\\-]{3,}")
{
    idmlib::util::IDMAnalyzerConfig csconfig = idmlib::util::IDMAnalyzerConfig::GetCommonConfig("","", "");
    csconfig.symbol = false;
    analyzer_ = new idmlib::util::IDMAnalyzer(csconfig);
    model_stop_set_.insert("09women");
    model_stop_set_.insert("g2000");
    model_stop_set_.insert("itisf4");
    model_stop_set_.insert("13kim");
    model_stop_set_.insert("nike360");
    model_stop_set_.insert("8080s");
    model_stop_set_.insert("1111");
    model_stop_set_.insert("1212");
    AddCategoryRule("^服装服饰>男装.*$", "男装");
    AddCategoryRule("^服装服饰>女装.*$", "女装");
    AddCategoryRule("^运动户外>户外服饰.*$", "户外服饰");
    AddCategoryRule("^运动户外>运动服饰.*$", "运动服饰");
    AddCategoryRule("^运动户外>运动鞋.*$", "运动鞋");
    AddCategoryRule("^运动户外>户外鞋.*$", "户外鞋");
    AddCategoryRule("^鞋包配饰>男鞋.*$", "男鞋");
    AddCategoryRule("^鞋包配饰>女鞋.*$", "女鞋");
    AddCategoryRule("^鞋包配饰>男包.*$", "男包");
    AddCategoryRule("^鞋包配饰>女包.*$", "女包");
    AddCategoryRule("^母婴童装>孕产妇>孕妇装$", "孕妇装");
    AddCategoryRule("^母婴童装>儿童服饰.*$", "童装");
    AddCategoryRule("^母婴童装>婴儿服饰.*$", "童装");
    error_model_regex_.push_back(boost::regex("^.*\\dcm"));
    error_model_regex_.push_back(boost::regex("\\d{2}0"));
    error_model_regex_.push_back(boost::regex("\\d{2}\\-\\d{2}"));
    error_model_regex_.push_back(boost::regex("\\d{1,3}\\-\\d{1,2}0"));
    error_model_regex_.push_back(boost::regex("[a-z]*201\\d"));
    error_model_regex_.push_back(boost::regex("201\\d[a-z]*"));
    error_model_regex_.push_back(boost::regex("200\\d[a-z]*"));
    error_model_regex_.push_back(boost::regex("[a-z]{4,}\\d"));
    error_model_regex_.push_back(boost::regex("123\\d{0,3}"));
    std::string odb_path = path_+"/odb";
    if(!boost::filesystem::exists(odb_path))
    {
        boost::filesystem::create_directories(odb_path);
    }
    odb_ = new OfferDb(odb_path);
    if(!odb_->open())
    {
        throw std::runtime_error("odb open error in bmn");
    }
    std::string cpath = path_+"/container";
    if(!boost::filesystem::exists(cpath))
    {
        boost::filesystem::create_directories(cpath);
        std::cerr<<"creating "<<cpath<<std::endl;
    }
    container_ = new BmnContainer(cpath);
}
BmnClustering::~BmnClustering()
{
    delete analyzer_;
    if(container_!=NULL) delete container_;
    if(odb_!=NULL) delete odb_;
}
void BmnClustering::AddCategoryKeyword(const std::string& keyword)
{
    word_t word;
    GetWord_(keyword, word);
    kdict_.insert(word);
}
void BmnClustering::AddCategoryRule(const std::string& category_regex, const std::string& group_name)
{
    category_rule_.push_back(std::make_pair(boost::regex(category_regex), group_name));
}
bool BmnClustering::IsValid(const Document& doc) const
{
    std::string category;
    doc.getString("Category", category);
    if(category.empty()) return false;
    std::string group_name;
    if(GetGroupName_(category, group_name)) return true;
    return false;
}
bool BmnClustering::GetGroupName_(const std::string& category, std::string& group_name) const
{
    for(std::size_t i=0;i<category_rule_.size();i++)
    {
        if(boost::regex_match(category, category_rule_[i].first))
        {
            group_name = category_rule_[i].second;
            return true;
        }
    }
    return false;
}
void BmnClustering::GetWord_(const std::string& text, word_t& word)
{
    UString title(text, UString::UTF_8);
    std::vector<idmlib::util::IDMTerm> terms;
    analyzer_->GetTermList(title, terms);
    for(std::size_t i=0;i<terms.size();i++)
    {
        word.push_back(terms[i].id);
    }
}
BmnContainer* BmnClustering::GetContainer_()
{
    return container_;
}

bool BmnClustering::Get(const std::string& oid, std::string& pid)
{
    return odb_->get(oid, pid);
}

bool BmnClustering::Append(const Document& doc, const std::vector<std::string>& brands, const std::vector<std::string>& keywords)
{
    std::string title;
    doc.getString("Title", title);
    std::string category;
    doc.getString("Category", category);
    std::string group_name;
    GetGroupName_(category, group_name);
    std::string model;
    ExtractModel_(title, model);
    if(model.empty()) return false;
    word_t word;
    GetWord_(title, word);
    if(word.empty()) return false;
    std::string sbrand;
    for(std::size_t i=0;i<brands.size();i++)
    {
        const std::string& brand = brands[i];
        if(brand.length()>sbrand.length()) sbrand = brand;
    }
    Container* container = GetContainer_();
    Value value(sbrand, model, group_name, keywords, doc, word);
    boost::unique_lock<boost::mutex> lock(mutex_);
    container->Append(value);
    return true;
}
bool BmnClustering::Finish(const Func& func, int thread_num)
{
    func_ = func;
    if(container_!=NULL) 
    {
        container_->Finish(boost::bind(&BmnClustering::Calculate_, this, _1), thread_num);
        delete container_;
        container_ = NULL;
    }
    odb_->flush();
    return true;
}
void BmnClustering::TooMany_(ValueArray& va)
{
    for(std::size_t i=0;i<va.size();i++)
    {
        Value& v = va[i];
        if(v.gid==0)
        {
            Output_(v);
        }
        else {
            break;
        }
    }
#ifdef BMN_DEBUG
    std::cout<<std::endl;
#endif
}
void BmnClustering::Calculate_(ValueArray& va)
{
    //std::cerr<<boost::this_thread::get_id()<<" "<<va.size()<<std::endl;
    if(va.empty()) return;
#ifdef BMN_DEBUG
    for(std::size_t i=0;i<va.size();i++)
    {
        va[i].gid = 0;
    }
#endif
    std::sort(va.begin(), va.end(), Value::GidCompare);
    if(va.size()>1000) 
    {
        TooMany_(va);
        return;
    }
    std::size_t gid0 = va.front().gid;
    if(gid0>0) return;
    //const std::string& brand = va.front().brand;
    const std::string& model = va.front().model;
    //static const std::size_t NGRAM_MAX_LEN = 10;
    typedef std::vector<std::size_t> posting_t;
    typedef std::map<word_t, posting_t> map_t;
    map_t map;
    std::size_t max_gid=va.back().gid;
    std::size_t count0 = 0;
    for(std::size_t i=0;i<va.size();i++)
    {
        Value& v = va[i];
        if(v.gid==0) count0 = i+1;
#ifdef BMN_DEBUG
        //std::string str;
        //v.doc.getString("Title", str);
        std::cout<<"["<<i<<"]"<<v.brand<<std::endl;
#endif
        for(std::size_t w=0;w<v.word.size();w++)
        {
            word_t frag(v.word.begin()+w, v.word.end());
            map[frag].push_back(i);
            //for(std::size_t l=2;l<=NGRAM_MAX_LEN;l++)
            //{
            //    std::size_t e=w+l;
            //    if(e>v.word.size()) break;
            //    word_t frag(v.word.begin()+w, v.word.begin()+e);
            //    map[frag].push_back(i);
            //}
        }
    }
#ifdef BMN_DEBUG
    //std::cerr<<"map finish"<<std::endl;
#endif
    typedef std::pair<std::size_t, std::size_t> DocPair;
    typedef std::vector<word_t> WordList;
    typedef boost::unordered_map<DocPair, WordList> MCS;
    MCS mcs;
    for(std::size_t i=0;i<va.size();i++)
    {
        Value& v = va[i];
        if(v.gid>0)
        {
            break;
        }
        std::size_t matrix[va.size()][v.word.size()];
        for(std::size_t x=0;x<va.size();x++)
        {
            for(std::size_t y=0;y<v.word.size();y++)
            {
                matrix[x][y] = 0;
            }
        }
        for(std::size_t w=0;w<v.word.size();w++)
        {
            for(std::size_t e=w+2;e<=v.word.size();e++)
            {
                //std::size_t len = e-w;
                word_t frag(v.word.begin()+w, v.word.begin()+e);
                bool found=false;
                for(map_t::const_iterator mit = map.lower_bound(frag);mit!=map.end();++mit)
                {
                    if(boost::algorithm::starts_with(mit->first, frag))
                    {
                        const posting_t& p = mit->second;
                        for(std::size_t j=0;j<p.size();j++)
                        {
                            std::size_t id = p[j];
                            if(id>i)
                            {
                                matrix[id][w] = e;
                            }
                        }
                        found=true;
                    }
                    else
                    {
                        break;
                    }
                }
                if(!found) break;
            }
        }
#ifdef BMN_DEBUG
        //for(std::size_t x=0;x<va.size();x++)
        //{
        //    std::cout<<"[M-"<<x<<"]";
        //    for(std::size_t y=0;y<v.word.size();y++)
        //    {
        //        std::cout<<matrix[x][y]<<",";
        //    }
        //    std::cout<<std::endl;
        //}
#endif
        for(std::size_t x=i+1;x<va.size();x++)
        {
            DocPair key(i, x);
            std::size_t y=0;
            while(true)
            {
                if(y>=v.word.size()) break;
                if(matrix[x][y]>0)
                {
                    word_t word(v.word.begin()+y, v.word.begin()+matrix[x][y]);
                    mcs[key].push_back(word);
                    y = matrix[x][y];
                }
                else
                {
                    ++y;
                }
            }
        }
    }
#ifdef BMN_DEBUG
    //std::cerr<<"mcs finish"<<std::endl;
#endif
    //for(map_t::iterator it = map.begin();it!=map.end();++it)
    //{
    //    const word_t& frag = it->first;
    //    posting_t& posting = it->second;
    //    std::sort(posting.begin(), posting.end());
    //    posting.erase(std::unique(posting.begin(), posting.end()), posting.end());
    //    for(std::size_t i=0;i<posting.size();i++)
    //    {
    //        for(std::size_t j=1;j<posting.size();j++)
    //        {
    //            DocPair key(posting[i], posting[j]);
    //            MCS::iterator mit = mcs.find(key);
    //            if(mit==mcs.end())
    //            {
    //                mcs.insert(std::make_pair(key, WordList(1, frag)));
    //            }
    //            else
    //            {
    //                WordList& word_list = mit->second;
    //                for(std::size_t w=0;w<word_list.size();w++)
    //                {
    //                    word_t& word = word_list[w];
    //                    if(boost::algorithm::contains(word, frag))
    //                    {
    //                        break;
    //                    }
    //                    else if(boost::algorithm::contains(frag, word))
    //                    {
    //                        word = frag;
    //                        break;
    //                    }
    //                }
    //            }
    //        }
    //    }
    //}
    std::vector<std::pair<double, DocPair> > doc_pair_score;
    std::pair<double, double> threshold(0.33, 0.5);
    if(model.length()==5)
    {
        threshold.first = 0.3;
        threshold.second = 0.4;
    }
    else if(model.length()>5)
    {
        threshold.first = 0.3;
        threshold.second = 0.3;
    }
    else if(model.length()>7)
    {
        threshold.first = 0.15;
        threshold.second = 0.15;
    }
    for(MCS::const_iterator it=mcs.begin();it!=mcs.end();++it)
    {
        const DocPair& doc_pair = it->first;
        const WordList& word_list = it->second;
        std::pair<std::size_t, std::size_t> term_lens(va[doc_pair.first].word.size(), va[doc_pair.second].word.size());
        std::size_t mcs_len=0;
        for(std::size_t i=0;i<word_list.size();i++)
        {
            mcs_len+=word_list[i].size();
        }
        std::size_t down = std::min(term_lens.first, term_lens.second);
        double mcs_ratio = (double)mcs_len/down;
        //double mcs_ratio = 2.0*mcs_len/(term_lens.first+term_lens.second);
        if(mcs_ratio<threshold.first) continue;
        std::vector<std::string> kcommon;
        const Value& vi = va[doc_pair.first];
        const Value& vj = va[doc_pair.second];
        double pricei = vi.GetPrice();
        double pricej = vj.GetPrice();
        if(pricei<=0.0||pricej<=0.0) continue;
        double priceratio = std::max(pricei, pricej)/std::min(pricei, pricej);
        if(priceratio>2.0) continue;
        if(vi.brand==vj.brand)
        {
            mcs_ratio *= 1.3;
        }
        //if(!vi.brand.empty()&&!vj.brand.empty()&&vi.brand!=vj.brand)
        //{
        //    continue;
        //}
        std::size_t i=0;
        std::size_t j=0;
        while(i<vi.keywords.size()&&j<vj.keywords.size())
        {
            if(vi.keywords[i]==vj.keywords[j]) 
            {
                kcommon.push_back(vi.keywords[i]);
                i++;
                j++;
            }
            else if(vi.keywords[i]<vj.keywords[j]) i++;
            else j++;
        }
        if(!vi.keywords.empty()&&!vj.keywords.empty())
        {
            if(kcommon.empty()&&model.length()<5)
            {
                continue;
            }
        }
        mcs_ratio = (double)(mcs_len+kcommon.size())/down;
        //mcs_ratio = 2.0*(mcs_len+kcommon.size())/(term_lens.first+term_lens.second);
        if(mcs_ratio<threshold.second) continue;
        doc_pair_score.push_back(std::make_pair(mcs_ratio, doc_pair));
    }
#ifdef BMN_DEBUG
    //std::cerr<<"doc pair finish"<<std::endl;
#endif
    std::sort(doc_pair_score.begin(), doc_pair_score.end(), std::greater<std::pair<double, DocPair> >());
    bool is_rebuild = false;
    if(max_gid==0) is_rebuild = true;
    std::size_t new_gid = max_gid+1;
    typedef std::map<std::pair<std::size_t, std::size_t>, double> GidSim;
    GidSim gid_sim;
    std::set<std::size_t> output_index_set;
    std::vector<std::size_t> output_index_list;
    for(std::size_t i=0;i<doc_pair_score.size();i++)
    {
        double score = doc_pair_score[i].first;
        const DocPair& pair = doc_pair_score[i].second;
        Value& v1 = va[pair.first];
        Value& v2 = va[pair.second];
        if(v1.gid==0||v2.gid==0)
        {
            if(v1.gid==0&&v2.gid>0)
            {
                v1.gid = v2.gid;
            }
            else if(v1.gid>0&&v2.gid==0)
            {
                v2.gid = v1.gid;
            }
            else
            {
                v1.gid = new_gid;
                v2.gid = new_gid;
                ++new_gid;
            }
        }
        else if(v1.gid!=v2.gid)
        {
            if(is_rebuild)
            {
                std::pair<std::size_t, std::size_t> simkey(v1.gid, v2.gid);
                if(simkey.first>simkey.second) std::swap(simkey.first, simkey.second);
                GidSim::iterator gsit = gid_sim.find(simkey);
                if(gsit==gid_sim.end()) gid_sim.insert(std::make_pair(simkey, score));
                else
                {
                    if(score<gsit->second) gsit->second = score;
                }
            }
            //ignore else
        }
#ifdef BMN_DEBUG
        //std::string t1;
        //v1.doc.getString("Title", t1);
        //std::string t2;
        //v2.doc.getString("Title", t2);
        //std::cout<<"[R]"<<t1<<","<<t2<<","<<score<<std::endl;
#endif
    }
#ifdef BMN_DEBUG
    //std::cout<<"[REND]"<<std::endl;
#endif
    if(is_rebuild) //to merge group
    {
        typedef boost::unordered_map<std::size_t, std::size_t> GidMap;
        GidMap gid_map;
        for(GidSim::iterator gsit=gid_sim.begin();gsit!=gid_sim.end();++gsit)
        {
            double score = gsit->second;
            if(score<=0.0) continue;
            std::size_t gid1 = gsit->first.first;
            std::size_t gid2 = gsit->first.second;
            std::size_t tgid = gid1;
            GidMap::const_iterator gmit = gid_map.find(gid2);
            if(gmit!=gid_map.end())
            {
                tgid = gmit->second;
            }
            gid_map[gid2] = gid1;
        }
        for(std::size_t i=0;i<va.size();i++)
        {
            Value& v = va[i];
            std::size_t gid = v.gid;
            GidMap::const_iterator gmit = gid_map.find(gid);
            if(gmit!=gid_map.end())
            {
                v.gid = gmit->second;
            }
        }
    }
    for(std::size_t i=0;i<count0;i++)
    {
        Value& v = va[i];
        if(v.gid==0)
        {
            v.gid = new_gid;
            new_gid++;
        }
        Output_(v);
    }
#ifdef BMN_DEBUG
    std::cout<<std::endl;
#endif
}

void BmnClustering::Output_(const Value& v) const
{
#ifdef BMN_DEBUG
    std::string title;
    v.doc.getString("Title", title);
    std::cout<<"[G]"<<title<<" : ["<<v.category<<"|"<<v.GetPrice()<<"] "<<v.gid<<std::endl;
#endif
    ScdDocument doc(v.doc, UPDATE_SCD);
    if(v.gid>0)
    {
        std::stringstream ss;
        ss<<"http://www.b5m.com/"<<v.category<<"/"<<v.brand<<"/"<<v.model<<"/"<<v.gid;
        std::string url = ss.str();
        std::string pid = B5MHelper::GetPidByUrl(url);
        doc.property("uuid") = str_to_propstr(pid);
        std::string oid;
        doc.getString("DOCID", oid);
        if(!oid.empty())
        {
            odb_->insert(oid, pid);
        }
    }
    func_(doc);
}

void BmnClustering::ExtractModel_(const std::string& text, std::string& model) const
{
    std::string stitle = text;
    boost::algorithm::to_lower(stitle);
    boost::algorithm::trim(stitle);
    boost::sregex_token_iterator iter(stitle.begin(), stitle.end(), model_regex_, 0);
    boost::sregex_token_iterator end;
    std::vector<std::string> models;
    for( ; iter!=end; ++iter)
    {
        std::string candidate = *iter;
        boost::algorithm::to_lower(candidate);
        if(model_stop_set_.find(candidate)!=model_stop_set_.end()) continue;
        if(candidate[0]=='-' || candidate[candidate.length()-1]=='-') continue;
        uint32_t symbol_count = 0;
        uint32_t digit_count = 0;
        uint32_t alpha_count = 0;
        for(uint32_t i=0;i<candidate.length();i++)
        {
            char c = candidate[i];
            if(c=='-')
            {
                symbol_count++;
            }
            else if(c>='0'&&c<='9')
            {
                digit_count++;
            }
            else
            {
                alpha_count++;
            }
        }
        if(symbol_count>1) continue;
        bool has_digit = (digit_count!=0);
        bool all_digit = (symbol_count==0&&alpha_count==0&&digit_count>0);
        if(!has_digit) continue; 
        if(!all_digit&&candidate.length()<4) continue;
        //if(all_digit&&candidate.length()<=4) continue;
        if(has_digit&&!all_digit)
        {
            if(boost::algorithm::starts_with(stitle, candidate)) continue;
            if(alpha_count/digit_count>=3) continue;
        }
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
    for(uint32_t i=0;i<models.size();i++)
    {
        if(models[i].length()>model.length()) model = models[i];
    }
}

