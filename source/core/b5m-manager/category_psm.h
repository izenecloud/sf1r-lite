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
        typedef boost::unordered_map<uint128_t, uint128_t> ResultMap;
        struct BufferItem
        {
            std::string title;
            double price;
        };
        typedef std::vector<BufferItem> BufferValue;
        typedef std::pair<std::string, std::string> BufferKey;
        typedef boost::unordered_map<BufferKey, BufferValue> Buffer;
    public:
        //typedef idmlib::dd::DupDetector<uint32_t, uint32_t, Attach> DDType;
        //typedef DDType::GroupTableType GroupTableType;
        //typedef idmlib::dd::PSM<64, 3, 24, std::string, std::string, Attach> PsmType;
        CategoryPsm():
        model_regex_("[a-zA-Z\\d\\-]{4,}")
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
            category_list_.push_back(std::make_pair(boost::regex("^服装服饰.*$"), "服装服饰"));
            category_list_.push_back(std::make_pair(boost::regex("^运动户外>户外服饰.*$"), "服装服饰"));
            category_list_.push_back(std::make_pair(boost::regex("^运动户外>运动鞋.*$"), "鞋子"));
            category_list_.push_back(std::make_pair(boost::regex("^运动户外>户外鞋.*$"), "鞋子"));
            category_list_.push_back(std::make_pair(boost::regex("^鞋包配饰>男鞋.*$"), "鞋子"));
            category_list_.push_back(std::make_pair(boost::regex("^鞋包配饰>女鞋.*$"), "鞋子"));
            category_list_.push_back(std::make_pair(boost::regex("^鞋包配饰>男包.*$"), "包包"));
            category_list_.push_back(std::make_pair(boost::regex("^鞋包配饰>女包.*$"), "包包"));
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

        bool TryInsert(const Document& doc)
        {
            std::string scategory;
            doc.getString("Category", scategory);
            std::string ncategory;
            if(!CategoryMatch(scategory, ncategory)) return false;
            stat_.first++;
            std::string stitle;
            doc.getString("Title", stitle);
            if(stitle.empty()) return false;
            std::string ssource;
            doc.getString("Source", ssource);
            std::string sdocid;
            doc.getString("DOCID", sdocid);
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
                bool has_digit = false;
                bool all_digit = true;
                for(uint32_t i=0;i<candidate.length();i++)
                {
                    char c = candidate[i];
                    if(c>='0'&&c<='9')
                    {
                        has_digit = true;
                    }
                    else
                    {
                        all_digit = false;
                    }
                }
                if(!has_digit) continue;
                if(all_digit&&candidate.length()<=4) continue;
                if(has_digit&&!all_digit)
                {
                    if(boost::algorithm::starts_with(stitle, candidate)) continue;
                    uint32_t start_alpha_size = 0;
                    uint32_t end_digit_size = 0;
                    for(uint32_t i=0;i<candidate.length();i++)
                    {
                        char c = candidate[i];
                        if(c>='a'&&c<='z')
                        {
                            start_alpha_size++;
                        }
                        else
                        {
                            break;
                        }
                    }
                    uint32_t i=candidate.length()-1;
                    while(true)
                    {
                        char c = candidate[i];
                        if(c>='0'&&c<='9') end_digit_size++;
                        else break;
                        if(i==0) break;
                        i--;
                    }
                    if(start_alpha_size+end_digit_size==candidate.length())
                    {
                        if(start_alpha_size>=4&&end_digit_size<=2)
                        {
                            continue;
                        }
                        if(end_digit_size>0)
                        {
                            std::string end_digit = candidate.substr(candidate.length()-end_digit_size);
                            if(IsYear_(end_digit)) continue;
                        }
                    }
                }
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
            BufferItem item;
            item.title = stitle;
            if(!pprice.GetMid(item.price)) return false;
            BufferKey key(ncategory, model);
            buffer_[key].push_back(item);
            stat_.second++;
            //std::vector<std::pair<std::string, double> > doc_vector;
            //Attach attach;
            //attach.models = models;
            //std::vector<idmlib::util::IDMTerm> term_list;
            //UString title(stitle, UString::UTF_8);
            //analyzer_->GetTermList(title, term_list);

            //doc_vector.reserve(term_list.size());

//#ifdef PM_CLUST_TEXT_DEBUG
            //std::string stitle;
            //title.convertString(stitle, izenelib::util::UString::UTF_8);
            //std::cout<<"[Title] "<<stitle<<std::endl<<"[OTokens] ";
            //for (uint32_t i=0;i<term_list.size();i++)
            //{
                //std::cout<<"["<<term_list[i].TextString()<<","<<term_list[i].tag<<"],";
            //}
            //std::cout<<std::endl;
//#endif
            //if(term_list.empty()) return false;

            //typedef boost::unordered_map<std::string, double> WeightMap;
            //WeightMap wm;

            //for (uint32_t i=0;i<term_list.size();i++)
            //{
                //const std::string& str = term_list[i].TextString();
                //if( stop_set_.find(str)!=stop_set_.end()) continue;
                //WeightMap::iterator wit = wm.find(str);
                //double weight = 1.0;
                //if(wit==wm.end())
                //{
                    //wm.insert(std::make_pair(str, weight));
                //}
                //else
                //{
                    //wit->second+=weight;
                //}
                ////if( app.find(str)!=app.end()) continue;
                ////app.insert(str);
                ////char tag = term_list[i].tag;
                ////double weight = GetWeight_(term_list.size(), term_list[i].text, tag);
                ////if( weight<=0.0 ) continue;
                ////double weight = 1.0;
                ////doc_vector.push_back(std::make_pair(str, weight));
            //}
            //for(uint32_t i=0;i<models.size();i++)
            //{
                //const std::string& str = models[i];
                //WeightMap::iterator wit = wm.find(str);
                //double weight = 4.0;
                //if(wit==wm.end())
                //{
                    //wm.insert(std::make_pair(str, weight));
                //}
                //else
                //{
                    //wit->second+=weight;
                //}
            //}
            //for(WeightMap::const_iterator wit = wm.begin();wit!=wm.end();wit++)
            //{
                //doc_vector.push_back(std::make_pair(wit->first, wit->second));
            //}

//#ifdef PM_CLUST_TEXT_DEBUG
            //std::cout<<"[ATokens] ";
            //for (uint32_t i=0;i<doc_vector.size();i++)
            //{
                //std::cout<<"["<<doc_vector[i].first<<","<<doc_vector[i].second<<"],";
            //}
            //std::cout<<std::endl;
//#endif

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

        bool Flush(ResultMap& result)
        {
            //std::cerr<<"[STAT]"<<stat_.first<<","<<stat_.second<<std::endl;
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
        bool IsYear_(const std::string& str) const
        {
            if(str.length()!=4) return false;
            if(boost::algorithm::starts_with(str, "201")) return true;
            return false;
        }

    private:
        std::string path_;
        std::vector<std::pair<boost::regex, std::string> > category_list_;
        boost::regex model_regex_;
        idmlib::util::IDMAnalyzer* analyzer_;
        //GroupTableType* table_;
        //DDType* dd_;
        //std::vector<CacheItem> cache_list_;
        Buffer buffer_;
        boost::unordered_set<std::string> stop_set_;
        boost::unordered_set<std::string> model_stop_set_;

        std::pair<std::size_t, std::size_t> stat_;
        boost::mutex mutex_;
    };

}


#endif


