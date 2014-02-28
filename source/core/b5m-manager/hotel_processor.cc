#include "hotel_processor.h"
#include "scd_doc_processor.h"

using namespace sf1r;
using namespace sf1r::b5m;
//#define HOTEL_DEBUG

HotelProcessor::HotelProcessor(const B5mM& b5mm) : nsim_threshold_(0.9), b5mm_(b5mm), limit_(0), postfix_index_(1)
{
    idmlib::util::IDMAnalyzerConfig csconfig = idmlib::util::IDMAnalyzerConfig::GetCommonConfig("","", "");
    csconfig.symbol = false;
    analyzer_ = new idmlib::util::IDMAnalyzer(csconfig);
    ae_ = new addrlib::AddressExtract;
    if(!ae_->Train(b5mm_.addr_knowledge))
    {
        throw std::runtime_error("addr train error");
    }
    empty_city_ = "__EMPTY_CITY__";
    InsertPostfixNoise_("酒店");
    InsertPostfixNoise_("大酒店");
    InsertPostfixNoise_("快捷酒店");
    InsertPostfixNoise_("商务酒店");
    InsertPostfixNoise_("连锁酒店");
    InsertPostfixNoise_("宾馆");
    InsertPostfixNoise_("旅馆");
    InsertPostfixNoise_("招待所");
    InsertPostfixNoise_("客栈");
    InsertPostfixNoise_("度假村");
    InsertPostfixNoise_("会所");
    InsertPostfixNoise_("商务会所");
    InsertPostfixNoise_("饭店");
    InsertPostfixNoise_("大饭店");
    InsertPostfixNoise_("山庄");
}
HotelProcessor::~HotelProcessor()
{
    delete analyzer_;
    delete ae_;
}
void HotelProcessor::InsertPostfixNoise_(const std::string& text)
{
    word_t word;
    GetWord_(text, word);
    word_t rword(word.rbegin(), word.rend());
    postfix_noise_.insert(std::make_pair(rword, postfix_index_++));
}
bool HotelProcessor::Generate(const std::string& mdb_instance)
{
    const std::string& scd_path = b5mm_.scd_path;
    namespace bfs = boost::filesystem;
    std::vector<std::string> scd_list;
    B5MHelper::GetIUScdList(scd_path, scd_list);
    if(scd_list.empty()) return false;
    std::string work_dir = mdb_instance+"/container";
    B5MHelper::PrepareEmptyDir(work_dir);
    std::string writer_file = work_dir+"/file1";
    writer_.reset(new Writer(writer_file));
    writer_->Open();
    ScdDocProcessor processor(boost::bind(&HotelProcessor::ProcessDoc_, this, _1), 1);
    if(limit_>0) processor.SetLimit(limit_);
    processor.AddInput(scd_path);
    processor.Process();
    writer_->Close();
    Sorter sorter;
    sorter.Sort(writer_->GetPath());
    Reader reader(writer_->GetPath());
    reader.Open();
    std::size_t key=0;
    typedef std::vector<Document> Documents;
    std::size_t lkey=0;
    const std::string& b5mo_path = b5mm_.b5mo_path;
    B5MHelper::PrepareEmptyDir(b5mo_path);
    owriter_.reset(new ScdWriter(b5mo_path, UPDATE_SCD));
    const std::string& b5mp_path = b5mm_.b5mp_path;
    B5MHelper::PrepareEmptyDir(b5mp_path);
    pwriter_.reset(new ScdWriter(b5mp_path, UPDATE_SCD));
    LOG(INFO)<<"reader size "<<reader.Count()<<std::endl;
    std::size_t p=0;
    B5mThreadPool<Documents> pool(b5mm_.thread_num, boost::bind(&HotelProcessor::DoClustering_, this, _1));
    Documents* docs = new Documents;
    Document doc;
    while(reader.Next(key, doc))
    {
        ++p;
        if(p%1000==0)
        {
            LOG(INFO)<<"Processing "<<p<<std::endl;
        }
        //if(p<310000) continue;
        if(key!=lkey&&!docs->empty())
        {
            pool.schedule(docs);
            docs = new Documents;
        }
        docs->push_back(doc);
        lkey = key;
    }
    reader.Close();
    if(!docs->empty())
    {
        pool.schedule(docs);
    }
    else
    {
        delete docs;
    }
    pool.wait();
    owriter_->Close();
    pwriter_->Close();
    return true;
}


void HotelProcessor::GetWord_(const std::string& name, word_t& word)
{
    UString title(name, UString::UTF_8);
    std::vector<idmlib::util::IDMTerm> terms;
    analyzer_->GetTermList(title, terms);
    for(std::size_t i=0;i<terms.size();i++)
    {
        word.push_back(terms[i].id);
#ifdef HOTEL_DEBUG
        term_text_[terms[i].id] = terms[i].TextString();
#endif
    }
}

bool HotelProcessor::Increase_(WordCount& map, const word_t& word, std::size_t c)
{
    WordCount::iterator it = map.find(word);
    if(it==map.end()) 
    {
        map.insert(std::make_pair(word, c));
        return true;
    }
    else it->second+=c;
    return false;
}

std::string HotelProcessor::GetText_(const word_t& word)
{
    std::string r;
    for(uint32_t i=0;i<word.size();i++)
    {
        r+=term_text_[word[i]];
    }
    return r;
}

std::size_t HotelProcessor::NoiseMatch_(const word_t& word, const NoiseSet& nset, std::size_t& nindex)
{
    std::size_t r = 0;
    word_t frag;
    for(word_t::const_iterator it=word.begin();it!=word.end();++it)
    {
        frag.push_back(*it);
        NoiseSet::const_iterator ns_it = nset.lower_bound(frag);
        if(ns_it==nset.end())
        {
            break;
        }
        else
        {
            if(boost::algorithm::starts_with(ns_it->first, frag))
            {
                if(ns_it->first==frag)
                {
                    r = frag.size();
                    nindex = ns_it->second;
                }
                else
                {
                    //continue do loop
                }
            }
            else break;
        }
    }
    return r;
}

void HotelProcessor::DoClustering_(std::vector<Document>& docs)
{
    //std::cerr<<"clustering doc size "<<docs.size()<<std::endl;
    //static const double threshold = 0.85;
    static const term_t PREFIX = 0;
    static const term_t SUFFIX = 1;
    static const term_t PREFIX_NUM = 2;
    static const term_t SUFFIX_NUM = 3;
    static const term_t PREFIX_CLASS = 4;
    static const term_t SUFFIX_CLASS = 5;
    WordCount ps_count; //prefix and suffix count, prefix start with 0 while suffix start with 1
    WordScore word_score;
    WordText word_text;
    static const uint32_t max_ps_num = 4;
    std::vector<Item> items(docs.size());
    for(std::size_t i=0;i<docs.size();i++)
    {
        Document& doc = docs[i];
        Item& item = items[i];
        item.index = i;
        item.doc = doc;
        std::string name;
        doc.getString("Name", name);
#ifdef HOTEL_DEBUG
        //std::string city;
        //doc.getString("City", city);
        //std::cerr<<"["<<item.index<<"]"<<name<<"\t"<<city<<std::endl;
#endif
        std::size_t spos1 = name.find("(");
        std::size_t spos2 = name.find("（");
        std::size_t spos = spos1==std::string::npos ? spos2 : spos1;
        if(spos!=std::string::npos)
        {
            std::size_t epos1 = name.find(")");
            std::size_t epos2 = name.find("）");
            std::size_t epos = epos1==std::string::npos ? epos2 : epos1;
            if(epos!=std::string::npos)
            {
                std::string fname = name.substr(0, spos);
                fname += name.substr(epos+1);
                name = fname;
            }
        }
        GetWord_(name, items[i].word);
        const word_t& word = items[i].word;
        word_t ps(1, PREFIX);
        for(word_t::const_iterator it = word.begin(); it!=word.end(); ++it)
        {
            ps.push_back(*it);
            if(ps.size()>max_ps_num+1) break;
            word_t key(2);
            key[0] = PREFIX_CLASS;
            key[1] = ps.size()-1;
            if(Increase_(ps_count, ps))
            {
                Increase_(ps_count, key);
            }
            key[0] = PREFIX_NUM;
            Increase_(ps_count, key);
        }
        ps.resize(1);
        ps[0] = SUFFIX;
        for(word_t::const_reverse_iterator it = word.rbegin(); it!=word.rend(); ++it)
        {
            ps.push_back(*it);
            if(ps.size()>max_ps_num+1) break;
            word_t key(2);
            key[0] = SUFFIX_CLASS;
            key[1] = ps.size()-1;
            if(Increase_(ps_count, ps))
            {
                Increase_(ps_count, key);
            }
            key[0] = SUFFIX_NUM;
            Increase_(ps_count, key);
        }
        //std::string city;
        //doc.getString("City", city);
        //if(boost::algorithm::starts_with(name, city))
        //{
        //    name = name.substr(city.length());
        //}
        //GetNameWord_(name, items[i].word);
    }
    //for(uint32_t i=1;i<=max_ps_num;i++)
    //{
    //    word_t key(2);
    //    key[0] = PREFIX_NUM;
    //    key[1] = i;
    //    std::size_t up = 0;
    //    WordCount::const_iterator it = ps_count.find(key);
    //    if(it!=ps_count.end()) up = it->second;
    //    key[0] = PREFIX_CLASS;
    //    std::size_t down = 0;
    //    it = ps_count.find(key);
    //    if(it!=ps_count.end()) down = it->second;
    //    key[0] = PREFIX;
    //    word_score[key] = (double)up/down;
    //    key[0] = SUFFIX_NUM;
    //    up = 0;
    //    it = ps_count.find(key);
    //    if(it!=ps_count.end()) up = it->second;
    //    key[0] = SUFFIX_CLASS;
    //    down = 0;
    //    it = ps_count.find(key);
    //    if(it!=ps_count.end()) down = it->second;
    //    key[0] = SUFFIX;
    //    word_score[key] = (double)up/down;
    //}
    NoiseSet prefix_noise;
    std::size_t prefix_noise_index = 1;
    for(WordCount::const_iterator it=ps_count.begin();it!=ps_count.end();++it)
    {
        const word_t& key = it->first;
        term_t type = key[0];
        if(type!=PREFIX) continue;
        std::size_t count = it->second;
        uint32_t len = key.size()-1;
        if(len<2) continue;
        word_t skey(2);
        skey[0] = type==PREFIX ? PREFIX_NUM : SUFFIX_NUM;
        skey[1] = len;
        std::size_t num = ps_count[skey];
        skey[0] = type==PREFIX ? PREFIX_CLASS : SUFFIX_CLASS;
        std::size_t class_num = ps_count[skey];
        double prob = (double)count/num;
        //double expect = 1.0/class_num;
        word_t word(key.begin()+1, key.end());
        if(prob>=0.7 && class_num>=3)
        {
            prefix_noise.insert(std::make_pair(word, prefix_noise_index++));
        }
#ifdef HOTEL_DEBUG
        //if(prob/expect>2.0)
        //{
        //    std::cerr<<GetText_(key)<<" : "<<prob<<","<<expect<<std::endl;
        //}
#endif
    }
    for(std::size_t i=0;i<items.size();i++)
    {
        word_t& word = items[i].word;
        word_t rword(word.rbegin(), word.rend());
        std::size_t pindex = 0;
        std::size_t sindex = 0;
        std::size_t ni = NoiseMatch_(word, prefix_noise, pindex);
        std::size_t rni = NoiseMatch_(rword, postfix_noise_, sindex);
        if((ni>0||rni>0)&&(ni+rni<=word.size()-2))
        {
            if(ni>0)
            {
                items[i].prefix_noise_.assign(word.begin(), word.begin()+ni);
            }
            if(rni>0)
            {
                items[i].postfix_noise_.assign(rword.begin(), rword.begin()+rni);
            }
            word_t nword(word.begin()+ni, word.end()-rni);
            word.swap(nword);
        }
    }
    ForwardMap fmap;
    FindSimilar_(items, fmap);
    std::size_t findex=0;
    std::vector<std::string> owrite_docs;
    std::vector<std::string> pwrite_docs;
    for(ForwardMap::const_iterator it=fmap.begin();it!=fmap.end();++it)
    {
        std::vector<Document> docs;
        Item& t = items[it->first];
        for(std::size_t i=0;i<it->second.size();i++)
        {
            std::size_t index = (it->second)[i];
            Item& item = items[index];
            item.doc.property("uuid") = t.doc.property("DOCID");
            std::string doctext;
            ScdWriter::DocToString(item.doc, doctext);
            owrite_docs.push_back(doctext);
            //owriter_->Append(item.doc);
            docs.push_back(item.doc);
#ifdef HOTEL_DEBUG
            std::string name;
            item.doc.getString("Name", name);
            std::string addr;
            item.doc.getString("Address", addr);
            std::string city;
            item.doc.getString("City", city);
            std::string brand;
            item.doc.getString("Brand", brand);
            std::cerr<<"[C"<<findex<<"-"<<it->first<<"]["<<item.index<<"]"<<name<<"\t"<<addr<<"\t["<<city<<"]\t{"<<brand<<"}"<<std::endl;
#endif
        }
        Document pdoc;
        GenP_(docs, pdoc);
        //std::string pimg;
        //pdoc.getString("Img", pimg);
        //if(!pimg.empty())
        //{
        //    pwriter_->Append(pdoc);
        //}
        std::string doctext;
        ScdWriter::DocToString(pdoc, doctext);
        pwrite_docs.push_back(doctext);
        //pwriter_->Append(pdoc);
        findex++;
    }
    {
        boost::unique_lock<boost::mutex> lock(omutex_);
        for(std::size_t i=0;i<owrite_docs.size();i++)
        {
            owriter_->Append(owrite_docs[i]);
        }
    }
    {
        boost::unique_lock<boost::mutex> lock(pmutex_);
        for(std::size_t i=0;i<pwrite_docs.size();i++)
        {
            pwriter_->Append(pwrite_docs[i]);
        }
    }
#ifdef HOTEL_DEBUG
    std::cerr<<std::endl;
#endif

}

bool HotelProcessor::IsSameNoise_(const Item& x, const Item& y)
{
    if(!x.prefix_noise_.empty()&&!y.prefix_noise_.empty())
    {
        if(boost::algorithm::starts_with(x.prefix_noise_, y.prefix_noise_) || boost::algorithm::starts_with(y.prefix_noise_, x.prefix_noise_))
        {
        }
        else
        {
            return false;
        }
    }
    if(!x.postfix_noise_.empty()&&!y.postfix_noise_.empty())
    {
        if(boost::algorithm::starts_with(x.postfix_noise_, y.postfix_noise_) || boost::algorithm::starts_with(y.postfix_noise_, x.postfix_noise_))
        {
        }
        else
        {
            return false;
        }
    }
    return true;
}
double HotelProcessor::NameSimDetail_(std::size_t x, std::size_t y, std::size_t c)
{
    std::size_t down = std::min(x, y);
    if(c>down) c=down;
    return (double)c/down;
}

double HotelProcessor::NameSim_(const Item& x, const Item& y, const MCS& mcs)
{
    std::string xbrand;
    std::string ybrand;
    x.doc.getString("Brand", xbrand);
    y.doc.getString("Brand", ybrand);
    std::string xname;
    std::string yname;
    x.doc.getString("Name", xname);
    y.doc.getString("Name", yname);
    if(xbrand==xname)
    {
        xbrand.clear();
    }
    if(ybrand==yname)
    {
        ybrand.clear();
    }
    if(!xbrand.empty()&&!ybrand.empty())
    {
        if(xbrand==ybrand)
        {
            return 1.0;
        }
        else
        {
            return 0.0;
        }
    }
    DocPair key(x.index, y.index);
    if(key.first>key.second) std::swap(key.first, key.second);
    MCS::const_iterator it = mcs.find(key);
    if(it==mcs.end()) return 0.0;
    if(!IsSameNoise_(x, y)) return 0.0;
    std::size_t common_len=it->second;
    return NameSimDetail_(x.word.size(), y.word.size(), common_len);
#ifdef HOTEL_DEBUG
    //std::string xcity;
    //std::string ycity;
    //x.doc.getString("City", xcity);
    //y.doc.getString("City", ycity);
    //if(ratio>=0.8)
    //{
    //    std::cerr<<"[NSIM]"<<xname<<","<<yname<<":"<<ratio<<"\t("<<xcity<<","<<ycity<<")"<<std::endl;
    //}
#endif
}

double HotelProcessor::Sim_(const Item& x, const Item& y, const MCS& mcs)
{
    double asim = 1.0;//todo
    double nsim = NameSim_(x, y, mcs);
    return asim*nsim;
}

void HotelProcessor::FindSimilar_(const std::vector<Item>& items, ForwardMap& fmap)
{
    map_t map;
    std::size_t max_word_len = 0;
    for(std::size_t i=0;i<items.size();i++)
    {
        const Item& item = items[i];
        if(item.word.size()>max_word_len)
        {
            max_word_len = item.word.size();
        }
        for(std::size_t w=0;w<item.word.size();w++)
        {
            word_t frag(item.word.begin()+w, item.word.end());
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
    std::vector<std::string> name_list(items.size());
    std::vector<std::string> brand_list(items.size());
    for(std::size_t i=0;i<items.size();i++)
    {
        items[i].doc.getString("Name", name_list[i]);
        items[i].doc.getString("Brand", brand_list[i]);
        if(name_list[i]==brand_list[i])
        {
            brand_list[i].clear();
        }
    }
    size_t matrix_count = items.size()*max_word_len;
    std::size_t* matrix = new std::size_t[matrix_count];
    typedef boost::unordered_map<std::size_t, std::size_t> IdMap;
    IdMap idmap1;
    ForwardMap fmap1;
    PairToForward<> ptf1(idmap1, fmap1);
    //MCS mcs;
    for(std::size_t i=0;i<items.size();i++)
    {
        const Item& item = items[i];
        std::size_t wsize = item.word.size();
        std::size_t matrix_use = items.size()*wsize;
        if(i%10==0)
        {
            //LOG(INFO)<<"item size "<<i<<","<<items.size()<<","<<wsize<<std::endl;
        }
        memset(matrix, 0, sizeof(std::size_t)*matrix_use);
        //std::size_t matrix[items.size()][item.word.size()];
        //for(std::size_t x=0;x<items.size();x++)
        //{
        //    for(std::size_t y=0;y<item.word.size();y++)
        //    {
        //        matrix[x][y] = 0;
        //    }
        //}
        //continue;
        boost::unordered_set<word_t> word_app;
        for(std::size_t w=0;w<item.word.size();w++)
        {
            for(std::size_t e=w+2;e<=item.word.size();e++)
            {
                //std::size_t len = e-w;
                word_t frag(item.word.begin()+w, item.word.begin()+e);
                if(word_app.find(frag)!=word_app.end()) continue;
                else word_app.insert(frag);
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
                                matrix[id*wsize+w] = e;
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
#ifdef HOTEL_DEBUG
        //for(std::size_t x=0;x<items.size();x++)
        //{
        //    std::cout<<"[M-"<<x<<"]";
        //    for(std::size_t y=0;y<item.word.size();y++)
        //    {
        //        std::cout<<matrix[x][y]<<",";
        //    }
        //    std::cout<<std::endl;
        //}
#endif
        for(std::size_t x=i+1;x<items.size();x++)
        {
            double nsim = 0.0;
            const std::string& abrand = brand_list[i];
            const std::string& bbrand = brand_list[x];
            if(!abrand.empty()&&!bbrand.empty())
            {
                if(abrand==bbrand) nsim = 1.0;
                else nsim = 0.0;
            }
            else
            {
                DocPair key(i, x);
                std::size_t value = 0;
                std::size_t y=0;
                while(true)
                {
                    if(y>=item.word.size()) break;
                    if(matrix[x*wsize+y]>0)
                    {
                        word_t word(item.word.begin()+y, item.word.begin()+matrix[x*wsize+y]);
                        std::size_t word_len = word.size();
                        value += word_len;
                        //mcs[key].push_back(word);
                        y = matrix[x*wsize+y];
                    }
                    else
                    {
                        ++y;
                    }
                }
                nsim = NameSimDetail_(item.word.size(), items[x].word.size(), value);
#ifdef HOTEL_DEBUG
                //std::cerr<<"[NSIMDE]"<<item.word.size()<<","<<items[x].word.size()<<","<<value<<" : "<<nsim<<std::endl;
#endif
            }
            if(nsim>=nsim_threshold_)
            {
                ptf1.Add(i, x);
#ifdef HOTEL_DEBUG
                //const std::string& aname = name_list[i];
                //const std::string& bname = name_list[x];
                //std::cerr<<"[NSIM]"<<aname<<"("<<GetText_(item.word)<<"),"<<bname<<"("<<GetText_(items[x].word)<<") : "<<nsim<<std::endl;
#endif
                //mcs.insert(std::make_pair(key, value));
            }
        }
    }
    delete[] matrix;
//    for(std::size_t i=0;i<items.size();i++)
//    {
//        for(std::size_t j=i+1;j<items.size();j++)
//        {
//            double nsim = NameSim_(items[i], items[j], mcs);
//#ifdef HOTEL_DEBUG
//            std::string xname;
//            std::string yname;
//            items[i].doc.getString("Name", xname);
//            items[j].doc.getString("Name", yname);
//            std::string xbrand;
//            std::string ybrand;
//            items[i].doc.getString("Brand", xbrand);
//            items[j].doc.getString("Brand", ybrand);
//            //std::cerr<<"[NSIM]["<<items[i].index<<","<<items[j].index<<"]"<<xname<<","<<yname<<":"<<nsim<<std::endl;
//            //std::cerr<<"[NSIM]"<<items[i].index<<","<<items[j].index<<":"<<nsim<<"\t{"<<xbrand<<","<<ybrand<<"}\t("<<xname<<","<<yname<<")"<<std::endl;
//#endif
//            if(nsim<nsim_threshold_) continue;
//            ptf1.Add(i, j);
//            //IdMap::iterator iditi = idmap.find(i);
//            //IdMap::iterator iditj = idmap.find(j);
//            //if(iditi==idmap.end()&&iditj==idmap.end())
//            //{
//            //    std::vector<std::size_t> fv(2);
//            //    fv[0] = i;
//            //    fv[1] = j;
//            //    fmap1[i] = fv;
//            //    idmap[i] = i;
//            //    idmap[j] = i;
//            //}
//            //else if(iditi==idmap.end()&&iditj!=idmap.end())
//            //{
//            //    std::size_t fkey = iditj->second;
//            //    idmap[i] = fkey;
//            //    fmap1[fkey].push_back(i);
//            //}
//            //else if(iditi!=idmap.end()&&iditj==idmap.end())
//            //{
//            //    std::size_t fkey = iditi->second;
//            //    idmap[j] = fkey;
//            //    fmap1[fkey].push_back(j);
//            //}
//            //else
//            //{
//            //    if(iditi->second!=iditj->second)
//            //    {
//            //        std::size_t fkey = iditi->second;
//            //        idmap[j] = fkey;
//            //        fmap1[fkey].insert(fmap1[fkey].end(), fmap1[iditj->second].begin(), fmap1[iditj->second].end());
//            //        fmap1.erase(iditj->second);
//            //    }
//            //}
//            //std::size_t a = i;
//            //std::size_t b = j;
//            //std::size_t t = a;
//            //IdMap::iterator idit = idmap.find(a);
//            //if(idit!=idmap.end()) t = idit->second;
//            //IdMap::iterator idit = idmap.find(b);
//            //idmap[b] = t;
//            //std::cerr<<"set idmap from "<<b<<" to "<<t<<","<<a<<std::endl;
//        }
//    }
    for(std::size_t i=0;i<items.size();i++)
    {
        ptf1.TryAddSingle(i);
    }
    ptf1.Flush();
    //std::cerr<<"start to do addr sim"<<std::endl;
    for(ForwardMap::iterator it = fmap1.begin();it!=fmap1.end();++it)
    {
        std::vector<std::size_t>& v = it->second;
        if(v.empty()) continue;
        std::size_t fkey = v.front();
        if(v.size()<2)
        {
            fmap[fkey] = v;
            continue;
        }
        std::vector<std::string> addr_list(v.size());
        for(std::size_t i=0;i<v.size();i++)
        {
            const Item& item = items[v[i]];
            item.doc.getString("Address", addr_list[i]);
#ifdef HOTEL_DEBUG
            //std::string name;
            //item.doc.getString("Name", name);
            //std::cerr<<"[A"<<i<<"]"<<addr_list[i]<<"\t"<<name<<"\t"<<item.index<<std::endl;
#endif
        }
        std::vector<std::pair<std::size_t, std::size_t> > r;
        ae_->FindSimilar(addr_list, r);
        IdMap idmap2;
        ForwardMap fmap2;
        PairToForward<> ptf2(idmap2, fmap2);
        for(std::size_t i=0;i<r.size();i++)
        {
            ptf2.Add(v[r[i].first], v[r[i].second]);
//            std::size_t a = v[r[i].first];
//            std::size_t b = v[r[i].second];
//            std::size_t t = a;
//#ifdef HOTEL_DEBUG
//            std::cerr<<"[AR"<<i<<"]"<<r[i].first<<","<<r[i].second<<std::endl;
//#endif
//            IdMap::const_iterator idit = idmap.find(a);
//            if(idit!=idmap.end()) t = idit->second;
//            idmap[b] = t;
        }
        for(std::size_t i=0;i<v.size();i++)
        {
            ptf2.TryAddSingle(v[i]);
//            IdMap::const_iterator idit = idmap.find(v[i]);
//            std::size_t t=v[i];
//            if(idit!=idmap.end()) t = idit->second;
//            fmap[t].push_back(v[i]);
        }
        ptf2.Flush();
        for(ForwardMap::const_iterator it=fmap2.begin(); it!=fmap2.end(); ++it)
        {
            const std::vector<std::size_t>& v2 = it->second;
            if(v2.empty()) continue;
            fmap[v2.front()] = v2;
        }
    }
}

void HotelProcessor::ProcessDoc_(ScdDocument& doc)
{
    std::string city;
    doc.getString("City", city);
    if(city.empty())
    {
        return;
        //city = empty_city_;
    }
    //std::string img;
    //doc.getString("Img", img);
    //if(img.empty()) return;
    std::size_t key=izenelib::util::HashFunction<std::string>::generateHash64(city);
    const Document& d = doc;
    writer_->Append(key, d);
}

void HotelProcessor::GenP_(const std::vector<Document>& docs, Document& pdoc)
{
    pdoc.property("DOCID") = docs.front().property("uuid");
    for(std::size_t i=0;i<docs.size();i++)
    {
        pdoc.merge(docs[i], false);
    }
    for(std::size_t i=0;i<docs.size();i++)
    {
        std::string img;
        docs[i].getString("Img", img);
        if(!img.empty())
        {
            pdoc.property("Img") = str_to_propstr(img);
            break;
        }
    }
    int64_t spread=0;
    bool spread_set = false;
    for(std::size_t i=0;i<docs.size();i++)
    {
        std::string sspread;
        docs[i].getString("Spread", sspread);
        if(!sspread.empty())
        {
            try {
                int32_t tspread = boost::lexical_cast<int64_t>(sspread);
                if(!spread_set)
                {
                    spread = tspread;
                    spread_set = true;
                }
                else
                {
                    if(tspread<spread) spread = tspread;
                }
            }
            catch(std::exception& ex)
            {
                LOG(ERROR)<<"parser spread value error {"<<sspread<<"}"<<std::endl;
            }
        }
    }
    if(spread_set)
    {
        pdoc.property("Spread") = spread;
    }
    pdoc.eraseProperty("uuid");
    pdoc.property("itemcount") = (int64_t)(docs.size());
}

