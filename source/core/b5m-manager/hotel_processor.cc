#include "hotel_processor.h"
#include "scd_doc_processor.h"

using namespace sf1r;
using namespace sf1r::b5m;
#define HOTEL_DEBUG

HotelProcessor::HotelProcessor(const B5mM& b5mm) : b5mm_(b5mm)
{
    idmlib::util::IDMAnalyzerConfig csconfig = idmlib::util::IDMAnalyzerConfig::GetCommonConfig("","", "");
    csconfig.symbol = false;
    analyzer_ = new idmlib::util::IDMAnalyzer(csconfig);
}
HotelProcessor::~HotelProcessor()
{
    delete analyzer_;
}
bool HotelProcessor::Generate(const std::string& mdb_instance)
{
    const std::string& scd_path = b5mm_.scd_path;
    namespace bfs = boost::filesystem;
    std::vector<std::string> scd_list;
    B5MHelper::GetIUScdList(scd_path, scd_list);
    if(scd_list.empty()) return false;
    std::string work_dir = mdb_instance+"/work_dir";
    B5MHelper::PrepareEmptyDir(work_dir);
    std::string writer_file = work_dir+"/file1";
    writer_.reset(new Writer(writer_file));
    writer_->Open();
    ScdDocProcessor processor(boost::bind(&HotelProcessor::ProcessDoc_, this, _1), 1);
    processor.AddInput(scd_path);
    processor.Process();
    writer_->Close();
    Sorter sorter;
    sorter.Sort(writer_->GetPath());
    Reader reader(writer_->GetPath());
    reader.Open();
    std::size_t key=0;
    Document doc;
    std::vector<Document> items;
    std::size_t lkey=0;
    const std::string& b5mo_path = b5mm_.b5mo_path;
    B5MHelper::PrepareEmptyDir(b5mo_path);
    owriter_.reset(new ScdWriter(b5mo_path, UPDATE_SCD));
    const std::string& b5mp_path = b5mm_.b5mp_path;
    B5MHelper::PrepareEmptyDir(b5mp_path);
    pwriter_.reset(new ScdWriter(b5mp_path, UPDATE_SCD));
    while(reader.Next(key, doc))
    {
        if(key!=lkey&&!items.empty())
        {
            DoClustering_(items);
            items.resize(0);
        }
        items.push_back(doc);
        lkey = key;
    }
    reader.Close();
    if(!items.empty())
    {
        DoClustering_(items);
    }
    owriter_->Close();
    pwriter_->Close();
}
void HotelProcessor::GetWord_(const std::string& name, word_t& word)
{
    UString title(name, UString::UTF_8);
    std::vector<idmlib::util::IDMTerm> terms;
    analyzer_->GetTermList(title, terms);
    for(std::size_t i=0;i<terms.size();i++)
    {
        word.push_back(terms[i].id);
    }
}
void HotelProcessor::DoClustering_(std::vector<Document>& docs)
{
    static const double threshold = 0.8;
    std::vector<Item> items(docs.size());
    for(std::size_t i=0;i<docs.size();i++)
    {
        Document& doc = docs[i];
        items[i].index = i;
        items[i].doc = doc;
        std::string name;
        doc.getString("Name", name);
        GetWord_(name, items[i].word);
    }
    map_t map;
    for(std::size_t i=0;i<items.size();i++)
    {
        Item& item = items[i];
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
    MCS mcs;
    for(std::size_t i=0;i<items.size();i++)
    {
        Item& item = items[i];
        std::size_t matrix[items.size()][item.word.size()];
        for(std::size_t x=0;x<items.size();x++)
        {
            for(std::size_t y=0;y<item.word.size();y++)
            {
                matrix[x][y] = 0;
            }
        }
        for(std::size_t w=0;w<item.word.size();w++)
        {
            for(std::size_t e=w+2;e<=item.word.size();e++)
            {
                //std::size_t len = e-w;
                word_t frag(item.word.begin()+w, item.word.begin()+e);
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
        for(std::size_t x=i+1;x<items.size();x++)
        {
            DocPair key(i, x);
            std::size_t y=0;
            while(true)
            {
                if(y>=item.word.size()) break;
                if(matrix[x][y]>0)
                {
                    word_t word(item.word.begin()+y, item.word.begin()+matrix[x][y]);
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
    typedef std::map<std::pair<std::size_t, std::size_t>, double> SimMap;
    SimMap simmap;
    for(std::size_t i=0;i<items.size();i++)
    {
        for(std::size_t j=i+1;j<items.size();j++)
        {
            double s = Sim_(items[i], items[j], mcs);
            if(s<threshold) continue;
            simmap.insert(std::make_pair(std::make_pair(i, j), s));
        }
    }
    typedef boost::unordered_map<std::size_t, std::size_t> IdMap;
    IdMap idmap;
    for(SimMap::const_iterator it = simmap.begin(); it!=simmap.end(); ++it)
    {
        std::size_t a = it->first.first;
        std::size_t b = it->first.second;
        std::size_t t = a;
        IdMap::const_iterator idit = idmap.find(a);
        if(idit!=idmap.end()) t = idit->second;
        idmap[b] = t;
    }
    typedef boost::unordered_map<std::size_t, std::vector<std::size_t> > ForwardMap;
    ForwardMap fmap;
    for(std::size_t i=0;i<items.size();i++)
    {
        IdMap::const_iterator it = idmap.find(i);
        std::size_t t=i;
        if(it!=idmap.end()) t = it->second;
        fmap[t].push_back(i);
    }
    for(std::size_t i=0;i<items.size();i++)
    {
        Document& doc = items[i].doc;
        IdMap::const_iterator it = idmap.find(i);
        if(it==idmap.end())
        {
            doc.property("uuid") = doc.property("DOCID");
        }
        else
        {
            doc.property("uuid") = items[it->second].doc.property("DOCID");
        }
        owriter_->Append(doc);
    }
    for(ForwardMap::const_iterator it=fmap.begin();it!=fmap.end();++it)
    {
        std::vector<Document> docs;
        for(std::size_t i=0;i<it->second.size();i++)
        {
            std::size_t index = (it->second)[i];
            docs.push_back(items[index].doc);
        }
        Document pdoc;
        GenP_(docs, pdoc);
        pwriter_->Append(pdoc);
    }
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
    if(!xbrand.empty()||!ybrand.empty())
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
    const WordList& word_list = it->second;
    std::size_t common_len=0;
    for(std::size_t i=0;i<word_list.size();i++)
    {
        common_len += word_list.size();
    }
    double ratio = 2.0*common_len/(x.word.size()+y.word.size());
#ifdef HOTEL_DEBUG
    std::cerr<<"[NSIM]"<<xname<<","<<yname<<":"<<ratio<<std::endl;
#endif
    return ratio;
}

double HotelProcessor::Sim_(const Item& x, const Item& y, const MCS& mcs)
{
    double asim = 0.0;//todo
    double nsim = NameSim_(x, y, mcs);
    return asim*nsim;
}

void HotelProcessor::ProcessDoc_(ScdDocument& doc)
{
    std::string city;
    doc.getString("City", city);
    if(city.empty())
    {
        city = "_EMPTY_CITY_";
    }
    std::size_t key=izenelib::util::HashFunction<std::string>::generateHash64(city);
    const Document& d = doc;
    writer_->Append(key, d);
}

void HotelProcessor::GenP_(const std::vector<Document>& docs, Document& pdoc)
{
}

