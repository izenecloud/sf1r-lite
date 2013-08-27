#ifndef SF1R_B5MMANAGER_CATEGORYPSM_H_
#define SF1R_B5MMANAGER_CATEGORYPSM_H_

#include "psm_helper.h"
#include <string>
#include <vector>
#include "b5m_types.h"
#include "b5m_helper.h"
#include <types.h>
#include <am/succinct/fujimap/fujimap.hpp>
#include <glog/logging.h>
#include <boost/unordered_map.hpp>
#include <idmlib/duplicate-detection/dup_detector.h>
#include <idmlib/duplicate-detection/psm.h>

namespace sf1r {

    class CategoryPsm
    {

        //struct CacheItem
        //{
            //uint128_t oid;
            //std::string title;
            //std::string source;
        //};

        //struct Attach
        //{
            //bool dd(const Attach& other) const
            //{
                //if(models.empty()||other.models.empty()) return false;
                //std::size_t i=0, j=0;
                //while(i<models.size()&&j<other.models.size())
                //{
                    //if(models[i]==other.models[j]) return true;
                    //if(models[i]<other.models[j]) ++i;
                    //else ++j;
                //}
                //return false;
            //}
            //friend class boost::serialization::access;
            //template<class Archive>
            //void serialize(Archive & ar, const unsigned int version)
            //{
                //ar & models;
            //}
            //std::vector<std::string> models;
        //};
        typedef std::vector<std::pair<std::string, double> > DocVector;
        typedef idmlib::dd::SimHash SimHash;
        typedef idmlib::dd::CharikarAlgo CharikarAlgo;
        struct BufferItem
        {
            std::string title;
            DocVector doc_vector;
            SimHash fp;
            double price;
            friend class boost::serialization::access;
            template<class Archive>
            void serialize(Archive & ar, const unsigned int version)
            {
                ar & title & doc_vector & fp & price;
            }
        };
        typedef std::vector<BufferItem> BufferValue;
        typedef std::pair<std::string, std::string> BufferKey;
        typedef boost::unordered_map<BufferKey, BufferValue> Buffer;

        struct Group
        {
            std::vector<BufferItem> items;
            uint32_t cindex;
            SimHash fp;
            double price;
            friend class boost::serialization::access;
            template<class Archive>
            void serialize(Archive & ar, const unsigned int version)
            {
                ar & items & cindex & fp & price;
            }

            bool operator<(const Group& g) const
            {
                return price<g.price;
            }
        };
        typedef boost::unordered_map<BufferKey, std::vector<Group> > ResultMap;
        typedef boost::unordered_map<std::string, double> WeightMap;
    public:
        //typedef idmlib::dd::DupDetector<uint32_t, uint32_t, Attach> DDType;
        //typedef DDType::GroupTableType GroupTableType;
        //typedef idmlib::dd::PSM<64, 3, 24, std::string, std::string, Attach> PsmType;
        CategoryPsm():
        model_regex_("[a-zA-Z\\d\\-]{4,}")
          , algo_()
          //, table_(NULL), dd_(NULL)
          , stat_(0,0)
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
            category_list_.push_back(std::make_pair(boost::regex("^服装服饰.*$"), "服装服饰"));
            category_list_.push_back(std::make_pair(boost::regex("^运动户外>户外服饰.*$"), "服装服饰"));
            category_list_.push_back(std::make_pair(boost::regex("^运动户外>运动服饰.*$"), "服装服饰"));
            category_list_.push_back(std::make_pair(boost::regex("^运动户外>运动鞋.*$"), "鞋子"));
            category_list_.push_back(std::make_pair(boost::regex("^运动户外>户外鞋.*$"), "鞋子"));
            category_list_.push_back(std::make_pair(boost::regex("^鞋包配饰>男鞋.*$"), "鞋子"));
            category_list_.push_back(std::make_pair(boost::regex("^鞋包配饰>女鞋.*$"), "鞋子"));
            category_list_.push_back(std::make_pair(boost::regex("^鞋包配饰>男包.*$"), "包包"));
            category_list_.push_back(std::make_pair(boost::regex("^鞋包配饰>女包.*$"), "包包"));
            category_list_.push_back(std::make_pair(boost::regex("^母婴童装>孕产妇>孕妇装$"), "孕妇装"));
            category_list_.push_back(std::make_pair(boost::regex("^母婴童装>儿童服饰.*$"), "童装"));
            category_list_.push_back(std::make_pair(boost::regex("^母婴童装>婴儿服饰.*$"), "童装"));
            error_model_regex_.push_back(boost::regex("\\d{2}cm"));
            error_model_regex_.push_back(boost::regex("\\d{2}\\-\\d{2}"));
            error_model_regex_.push_back(boost::regex("[a-z]+201\\d"));
            error_model_regex_.push_back(boost::regex("201\\d[a-z]+"));
            error_model_regex_.push_back(boost::regex("[a-z]{4,}\\d"));
        }
        ~CategoryPsm()
        {
            //if(table_!=NULL) delete table_;
            //if(dd_!=NULL) delete dd_;
            delete analyzer_;
        }
        bool Open(const std::string& path)
        {
            path_ = path;
            if(boost::filesystem::exists(path))
            {
                std::map<BufferKey, std::vector<Group> > rmap;
                izenelib::am::ssf::Util<>::Load(path, rmap);
                result_.insert(rmap.begin(), rmap.end());
            }
            //std::string work_dir = path+"/work_dir";
            //B5MHelper::PrepareEmptyDir(work_dir);
            //std::string dd_container = work_dir +"/dd_container";
            //std::string group_path = work_dir + "/group_table";
            //table_ = new GroupTableType(group_path);
            //table_->Load();
            //dd_ = new DDType(dd_container, table_);
            //dd_->SetFixK(10);

            //if(!dd_->Open())
            //{
                //std::cerr<<"DD open failed"<<std::endl;
                //return false;
            //}
            //std::string psm_path = path+"/psm";
            //B5MHelper::PrepareEmptyDir(psm_path);
            return true;
        }

        bool CategoryMatch(const std::string& category, std::string& ncategory) const
        {
            for(uint32_t i=0;i<category_list_.size();i++)
            {
                if(boost::regex_match(category, category_list_[i].first))
                {
                    ncategory = category_list_[i].second;
                    return true;
                }
            }
            return false;
        }

        bool Insert(const Document& doc)
        {
            std::string scategory;
            doc.getString("Category", scategory);
            std::string ncategory;
            if(!CategoryMatch(scategory, ncategory)) return false;
            stat_.first++;
            std::string ssource;
            doc.getString("Source", ssource);
            std::string sdocid;
            doc.getString("DOCID", sdocid);
            BufferKey key;
            key.first = ncategory;
            BufferItem item;
            if(!Analyze_(doc, key, item)) return false;
            boost::unique_lock<boost::mutex> lock(mutex_);
            buffer_[key].push_back(item);
            stat_.second++;


            //boost::unique_lock<boost::mutex> lock(mutex_);
            //if(cache_list_.empty()) cache_list_.resize(1);
            //uint32_t id = cache_list_.size();
            //dd_->InsertDoc(id, doc_vector, attach);
            //CacheItem cache;
            //cache.oid = B5MHelper::StringToUint128(sdocid);
            //cache.title = stitle;
            //cache.source = ssource;
            //cache_list_.push_back(cache);
            return true;
        }

        bool Search(const Document& doc, std::string& pid, std::string& ptitle)
        {
            std::string scategory;
            doc.getString("Category", scategory);
            std::string ncategory;
            if(!CategoryMatch(scategory, ncategory)) return false;
            BufferKey key;
            key.first = ncategory;
            BufferItem item;
            if(!Analyze_(doc, key, item)) return false;
            ResultMap::const_iterator it = result_.find(key);
            if(it==result_.end()) return false;
            const std::vector<Group>& groups = it->second;
            std::pair<double, uint32_t> dist_index(-1.0, 0);
            for(uint32_t j=0;j<groups.size();j++)
            {
                double dist = Distance_(item, groups[j]);
                if(dist_index.first<0.0 || dist<dist_index.first)
                {
                    dist_index.first = dist;
                    dist_index.second = j;
                }
            }
            if(dist_index.first>=0.0&&ValidDistance_(dist_index.first))
            {
                const Group& g = groups[dist_index.second];
                std::string url = "http://www.b5m.com/"+key.first+"/"+key.second+"/"+boost::lexical_cast<std::string>(dist_index.second);
                pid = B5MHelper::GetPidByUrl(url);
                const BufferItem& center = g.items[g.cindex];
                ptitle = center.title;
                return true;
            }
            else
            {
                return false;
            }

        }

        bool Flush(const std::string& path)
        {
            LOG(INFO)<<"[STAT]"<<stat_.first<<","<<stat_.second<<std::endl;
            LOG(INFO)<<"buffer size "<<buffer_.size()<<std::endl;
            
            std::size_t p=0;
            for(Buffer::const_iterator it = buffer_.begin();it!=buffer_.end();++it)
            {
                Clustering_(it->first, it->second);
                ++p;
                if(p%100000==0)
                {
                    LOG(INFO)<<"Processing clustering "<<p<<std::endl;
                }
            }
            buffer_.clear();
            for(ResultMap::const_iterator it = result_.begin();it!=result_.end();++it)
            {
                const BufferKey& key = it->first;
                std::cout<<"[Category]"<<key.first<<" [Model]"<<key.second<<" : "<<std::endl;
                const std::vector<Group>& groups = it->second;
                if(groups.size()>1)
                {
                    std::cout<<"MULTI GROUPS "<<groups.size()<<std::endl;
                }
                for(uint32_t i=0;i<groups.size();i++)
                {
                    const Group& group = groups[i];
                    const BufferItem& center = group.items[group.cindex];
                    std::cout<<"\t[Group]"<<center.title<<","<<center.price<<" : "<<group.items.size()<<std::endl;
                    if(group.items.size()>100)
                    {
                        std::cout<<"\tLARGE GROUP "<<group.items.size()<<std::endl;
                    }
                    for(uint32_t j=0;j<group.items.size();j++)
                    {
                        const BufferItem& item = group.items[j];
                        std::cout<<"\t\t[InGroup]"<<item.title<<","<<item.price<<std::endl;
                    }
                }
            }
            std::map<BufferKey, std::vector<Group> > rmap(result_.begin(), result_.end());
            izenelib::am::ssf::Util<>::Save(path, rmap);
            //dd_->RunDdAnalysis();
            //const std::vector<std::vector<uint32_t> >& group_info = table_->GetGroupInfo();
            //for(uint32_t i=0;i<group_info.size();i++)
            //{
                //uint32_t gid = i;
                //const std::vector<uint32_t>& group = group_info[gid];
                //if(group.size()<2) continue;
                //const CacheItem& base = cache_list_[group.front()];
                //for(uint32_t j=0;j<group.size();j++)
                //{
                    //const CacheItem& item = cache_list_[group[j]];
                    //result[item.oid] = base.oid;
                //}
                //std::cout<<"[GID]"<<gid<<std::endl;
                //for(uint32_t j=0;j<group.size();j++)
                //{
                    //const CacheItem& item = cache_list_[group[j]];
                    //std::cout<<"\t"<<item.title<<","<<item.source<<std::endl;
                //}
            //}
            return true;
        }

    private:
        void Tokenize_(const std::string& text, std::vector<std::string>& term_list)
        {
            UString title(text, UString::UTF_8);
            std::vector<idmlib::util::IDMTerm> terms;
            analyzer_->GetTermList(title, terms);
            term_list.reserve(terms.size());
            uint32_t i=0;
            while(true)
            {
                if(i>=terms.size()) break;
                UString ut = terms[i++].text;
                if(ut.isChineseChar(0))
                {
                    if(i<terms.size())
                    {
                        UString nut = terms[i].text;
                        if(nut.isChineseChar(0))
                        {
                            ut.append(nut);
                            ++i;
                        }
                    }
                }
                std::string str;
                ut.convertString(str, UString::UTF_8);
                term_list.push_back(str);

            }
        }
        bool Analyze_(const Document& doc, BufferKey& key, BufferItem& item)
        {
            std::string stitle;
            doc.getString("Title", stitle);
            if(stitle.empty()) return false;
            std::vector<std::string> models;
            boost::algorithm::to_lower(stitle);
            boost::algorithm::trim(stitle);
            boost::sregex_token_iterator iter(stitle.begin(), stitle.end(), model_regex_, 0);
            boost::sregex_token_iterator end;
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
                bool has_digit = (digit_count!=0);
                bool all_digit = (symbol_count==0&&alpha_count==0&&digit_count>0);
                if(!has_digit) continue; 
                if(all_digit&&candidate.length()<=4) continue;
                if(has_digit&&!all_digit)
                {
                    if(boost::algorithm::starts_with(stitle, candidate)) continue;
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
                //if(has_digit&&!all_digit)
                //{
                //    if(boost::algorithm::starts_with(stitle, candidate)) continue;
                //    uint32_t start_alpha_size = 0;
                //    uint32_t end_digit_size = 0;
                //    for(uint32_t i=0;i<candidate.length();i++)
                //    {
                //        char c = candidate[i];
                //        if(c>='a'&&c<='z')
                //        {
                //            start_alpha_size++;
                //        }
                //        else
                //        {
                //            break;
                //        }
                //    }
                //    uint32_t i=candidate.length()-1;
                //    while(true)
                //    {
                //        char c = candidate[i];
                //        if(c>='0'&&c<='9') end_digit_size++;
                //        else break;
                //        if(i==0) break;
                //        i--;
                //    }
                //    if(start_alpha_size+end_digit_size==candidate.length())
                //    {
                //        if(start_alpha_size>=4&&end_digit_size<=2)
                //        {
                //            continue;
                //        }
                //        if(end_digit_size>0)
                //        {
                //            std::string end_digit = candidate.substr(candidate.length()-end_digit_size);
                //            if(IsYear_(end_digit)) continue;
                //        }
                //    }
                //}
                models.push_back(candidate);
            }
            //std::cerr<<"[Category]"<<scategory<<std::endl;
            //std::cerr<<"[Title]"<<stitle<<std::endl;
            //for(uint32_t i=0;i<models.size();i++)
            //{
                //std::cerr<<"[Model]"<<models[i]<<std::endl;
            //}
            //std::sort(models.begin(), models.end());
            if(models.empty()) return false;
            std::string model;
            for(uint32_t i=0;i<models.size();i++)
            {
                if(models[i].length()>model.length()) model = models[i];
            }
            std::string sprice;
            doc.getString("Price", sprice);
            ProductPrice pprice;
            pprice.Parse(sprice);
            item.title = stitle;
            if(!pprice.GetMid(item.price)) return false;
            key.second = model;
            //BufferKey key(ncategory, model);
            DocVector& doc_vector = item.doc_vector;
            std::vector<std::string> term_list;
            Tokenize_(stitle, term_list);
            if(term_list.empty()) return false;
            doc_vector.reserve(term_list.size());
            WeightMap wm;
            for (uint32_t i=0;i<term_list.size();i++)
            {
                const std::string& str = term_list[i];
                if( stop_set_.find(str)!=stop_set_.end()) continue;
                WeightMap::iterator wit = wm.find(str);
                double weight = 1.0;
                if(wit==wm.end())
                {
                    wm.insert(std::make_pair(str, weight));
                }
                else
                {
                    wit->second+=weight;
                }
            }
            for(uint32_t i=0;i<models.size();i++)
            {
                const std::string& str = models[i];
                WeightMap::iterator wit = wm.find(str);
                double weight = 4.0;
                if(wit==wm.end())
                {
                    wm.insert(std::make_pair(str, weight));
                }
                else
                {
                    wit->second+=weight;
                }
            }
            std::vector<std::string> v;
            v.reserve(wm.size());
            std::vector<double> weights;
            weights.reserve(wm.size());
            for(WeightMap::const_iterator wit = wm.begin();wit!=wm.end();wit++)
            {
                doc_vector.push_back(std::make_pair(wit->first, wit->second));
                v.push_back(wit->first);
                weights.push_back(wit->second);
            }
            algo_.generate_document_signature(v, weights, item.fp);
            return true;
        }

        void Clustering_(const BufferKey& key, const BufferValue& value)
        {
            std::vector<Group> groups;
            for(uint32_t i=0;i<value.size();i++)
            {
                const BufferItem& item = value[i];
                std::pair<double, uint32_t> dist_index(-1.0, 0);
                for(uint32_t j=0;j<groups.size();j++)
                {
                    double dist = Distance_(item, groups[j]);
                    if(dist_index.first<0.0 || dist<dist_index.first)
                    {
                        dist_index.first = dist;
                        dist_index.second = j;
                    }
                }
                if(dist_index.first>=0.0&&ValidDistance_(dist_index.first))
                {
                    Group& g = groups[dist_index.second];
                    g.items.push_back(item);
                    GroupRefresh_(g);
                }
                else
                {
                    Group g;
                    g.items.push_back(item);
                    GroupRefresh_(g);
                    groups.push_back(g);
                }
            }
            std::stable_sort(groups.begin(), groups.end());
            result_[key] = groups;
        }

        void GroupRefresh_(Group& g)
        {
            if(g.items.empty()) return;
            WeightMap wm;
            double price = 0.0;
            for(uint32_t i=0;i<g.items.size();i++)
            {
                const DocVector& doc_vector = g.items[i].doc_vector;
                for(uint32_t j=0;j<doc_vector.size();j++)
                {
                    WeightMap::iterator it = wm.find(doc_vector[j].first);
                    if(it==wm.end())
                    {
                        wm.insert(std::make_pair(doc_vector[j].first, doc_vector[j].second));
                    }
                    else
                    {
                        it->second+=doc_vector[j].second;
                    }
                }
                price += g.items[i].price;
            }
            price /= g.items.size();
            g.price = price;
            std::vector<std::string> v;
            v.reserve(wm.size());
            std::vector<double> weights;
            weights.reserve(wm.size());
            for(WeightMap::const_iterator it = wm.begin();it!=wm.end();++it)
            {
                v.push_back(it->first);
                weights.push_back(it->second);
            }
            algo_.generate_document_signature(v, weights, g.fp);
            std::vector<std::pair<double, uint32_t> > dist_index;
            for(uint32_t i=0;i<g.items.size();i++)
            {
                double dist = Distance_(g.items[i], g);
                dist_index.push_back(std::make_pair(dist, i));
            }
            std::sort(dist_index.begin(), dist_index.end());
            g.cindex = dist_index[0].second;

        }


    private:

        bool IsYear_(const std::string& str) const
        {
            if(str.length()!=4) return false;
            if(boost::algorithm::starts_with(str, "201")) return true;
            return false;
        }

        double Distance_(const BufferItem& item, const Group& group) const
        {
            uint32_t hamming_dist = 0;

            for (uint32_t i = 0; i < SimHash::FP_SIZE; i++)
            {
                hamming_dist += __builtin_popcountl(item.fp.desc[i] ^ group.fp.desc[i]);
            }
            double maxp = item.price;
            double minp = group.price;
            if(minp>maxp) std::swap(maxp, minp);
            double pratio = maxp/minp;
            if(pratio>2.0) return 999.0;
            double hratio = std::log((double)hamming_dist+2.0);
            double dist = pratio*hratio;
            return dist;
        }

        bool ValidDistance_(double distance) const
        {
            return distance<4.5;
        }

    private:
        std::string path_;
        std::vector<std::pair<boost::regex, std::string> > category_list_;
        boost::regex model_regex_;
        std::vector<boost::regex> error_model_regex_;
        idmlib::util::IDMAnalyzer* analyzer_;
        //GroupTableType* table_;
        //DDType* dd_;
        //std::vector<CacheItem> cache_list_;
        Buffer buffer_;
        ResultMap result_;
        CharikarAlgo algo_;
        boost::unordered_set<std::string> stop_set_;
        boost::unordered_set<std::string> model_stop_set_;

        std::pair<std::size_t, std::size_t> stat_;
        boost::mutex mutex_;
    };

}


#endif


