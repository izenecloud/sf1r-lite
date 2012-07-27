#include <types.h>
#include <algorithm>
#include <am/succinct/fujimap/fujimap.hpp>
#include <common/ScdParser.h>
#include <common/ScdWriter.h>
#include <common/Utilities.h>
#include <document-manager/Document.h>
#include <iostream>
#include <string>

#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include <am/range/AmIterator.h>
#include <am/leveldb/Table.h>
//#include <am/sequence_file/ssfr.h>
#include <glog/logging.h>

//#define MERGER_DEBUG

namespace sf1r
{

class ScdMerger
{
public:
    
    struct ValueType
    {
        ValueType():type(NOT_SCD)
        {
        }

        ValueType(const Document& p1, int p2)
        :doc(p1), type(p2)
        {
        }

        Document doc;
        int type;

        bool empty() const
        {
            return doc.isEmpty();
        }

        void swap(ValueType& other)
        {
            std::swap(type, other.type);
            doc.swap(other.doc);
        }

        template<class Archive> void serialize(Archive & ar,
                const unsigned int version) {
            ar & doc & type;
        }
    };
    typedef boost::function<void (ValueType&, const ValueType&) > MergeFunctionType;
    typedef boost::function<void (Document&, int&) > OutputFunctionType;

    typedef uint128_t IdType;
    typedef uint64_t PositionType;

    struct PropertyConfig
    {
        std::string output_dir;
        std::string property_name;
        MergeFunctionType merge_function;
        OutputFunctionType output_function;
        bool output_if_no_position;
    };

    class CacheType
    {
    public:
        typedef boost::unordered_map<IdType, ValueType> MapType;
        typedef izenelib::am::leveldb::Table<IdType, ValueType> DbType;
        typedef MapType::iterator map_iterator;
        typedef MapType::const_iterator map_const_iterator;
        typedef std::pair<IdType, ValueType> PairType;
        struct iterator_node
        {
            uint8_t flag;
            map_iterator mit;
            DbType* db;
            IdType first;
            ValueType second;
            iterator_node()
            : flag(0)
            {
            }
            iterator_node(const map_iterator& p1)
            :flag(1), mit(p1), first(p1->first), second(p1->second)
            {
            }
            iterator_node(DbType* p0, const PairType& p1)
            :flag(2), db(p0), first(p1.first), second(p1.second)
            {
            }
            iterator_node(DbType* p0,const IdType& p1, const ValueType& p2)
            :flag(2), db(p0), first(p1), second(p2)
            {
            }


        };

        class iterator
        {
        public:
            iterator_node* operator->() const
            {
                return (iterator_node*)&node_;
            }

            iterator()
            : node_()
            {
            }

            iterator(const map_iterator& mit)
            : node_(mit)
            {
            }

            iterator(DbType* db, const PairType& value)
            : node_(db,value)
            {
            }

            iterator(DbType* db, const IdType& key, const ValueType& value)
            : node_(db,key,value)
            {
            }

            ~iterator()
            {
            }

            bool operator==(const iterator& it) const
            {
                return (*this)->flag==it->flag;
            }

            bool operator!=(const iterator& it) const
            {
                return !(operator==(it));
            }

        private:
            iterator_node node_;
        };
    public:
        CacheType(const std::string& path)
        : path_(path), db_(NULL)
        {
        }

        ~CacheType()
        {
            if(db_!=NULL)
            {
                delete db_;
                boost::filesystem::remove_all(path_);
            }
        }

        iterator end()
        {
            return iterator();
        }

        void insert(const std::pair<IdType, ValueType>& value)
        {
            //LOG(INFO)<<"insert "<<(std::size_t)value.first<<std::endl;
            if(db_==NULL)
            {
                map_.insert(value);
            }
            else
            {
                db_->insert(value.first, value.second);
            }
        }

        void insert_mem(const std::pair<IdType, ValueType>& value)
        {
            //LOG(INFO)<<"insert mem "<<(std::size_t)value.first<<std::endl;
            map_.insert(value);
        }

        iterator find(const IdType& key)
        {
            //LOG(INFO)<<"find "<<(std::size_t)key<<std::endl;
            map_iterator mit = map_.find(key);
            if(mit!=map_.end())
            {
                return iterator(mit);
            }
            else if(db_!=NULL)
            {
                ValueType value;
                if(db_->get(key, value))
                {
                    return iterator(db_, key, value);
                }
            }
            return iterator();
        }

        void erase(const iterator& it)
        {
            map_const_iterator mit = map_.find(it->first);
            if(mit!=map_.end())
            {
                map_.erase(mit);
            }

        }

        void update_back(iterator& it)
        {
            if(it->flag==1)
            {
                //it->mit->first = it->first;
                it->mit->second = it->second;
            }
            else
            {
                it->db->update(it->first, it->second);
            }
        }

        std::size_t in_mem_num() const
        {
            return map_.size();
        }

        DbType* GetDb()
        {
            if(db_==NULL)
            {
                db_ = new DbType(path_);
                db_->open();
            }
            return db_;
        }
        

    private:
        std::string path_;
        MapType map_;
        DbType* db_;

    };
    //typedef boost::unordered_map<IdType, ValueType> CacheType;
    typedef CacheType::iterator cache_iterator;
    typedef izenelib::am::succinct::fujimap::Fujimap<IdType, PositionType> PositionMap;

    class Cache
    {
        typedef std::map<std::string, CacheType*> DataType;
    public:
        Cache(const std::string& dir)
        :dir_(dir)
         , max_in_mem_(300000)
         , cache_num_(0)
         , cache_full_(false)
        {
        }

        ~Cache()
        {
            for(DataType::const_iterator it = data_.begin(); it!=data_.end();++it)
            {
                delete it->second;
            }
        }

        CacheType& operator[](const std::string& pname)
        {
            DataType::iterator it = data_.find(pname);
            if(it!=data_.end())
            {
                return *(it->second);
            }
            else
            {
                CacheType* ct = new CacheType(dir_+"/cache."+pname);
                data_.insert(std::make_pair(pname, ct));
                return *ct;
            }
        }


        std::size_t in_mem_num() const
        {
            std::size_t r = 0;
            for(DataType::const_iterator it = data_.begin(); it!=data_.end();++it)
            {
                r += it->second->in_mem_num();
            }
            return r;
        }

        //void clear()
        //{
            //data_.clear();
        //}

        void set_cache_full()
        {
            if(cache_full_) return;
            LOG(INFO)<<"set cache full!"<<std::endl;
            for(DataType::const_iterator it = data_.begin(); it!=data_.end();++it)
            {
                it->second->GetDb();
            }
            cache_full_ = true;
        }

        bool is_cache_full() const
        {
            return cache_full_;
        }
    private:
        std::string dir_;
        DataType data_;
        uint32_t max_in_mem_;
        uint32_t cache_num_;
        bool cache_full_;//has been full
    };

    struct PropertyConfigR
    {
        PropertyConfigR(const PropertyConfig& config)
        :output_dir(config.output_dir), property_name(config.property_name)
         , merge_function(config.merge_function)
         , output_function(config.output_function)
         , output_if_no_position(config.output_if_no_position)
         //, cache(output_dir+"/cache_"+property_name)
         , finish(false)
        {
            if(!boost::filesystem::exists(output_dir))
            {
                boost::filesystem::create_directories(output_dir);
            }
            std::string tmp_file = output_dir+"/fuji.tmp";
            boost::filesystem::remove_all(tmp_file);
            LOG(INFO)<<"init fujimap at "<<tmp_file<<std::endl;
            position_map.reset(new PositionMap(tmp_file.c_str()));
            position_map->initFP(32);
            position_map->initTmpN(30000000);
        }

        ~PropertyConfigR()
        {
            //delete position_map;
        }
        std::string output_dir;
        std::string property_name;
        MergeFunctionType merge_function;
        OutputFunctionType output_function;
        bool output_if_no_position;
        boost::shared_ptr<ScdWriter> i_writer;
        boost::shared_ptr<ScdWriter> u_writer;
        boost::shared_ptr<ScdWriter> d_writer;
        boost::shared_ptr<PositionMap> position_map;
        //CacheType cache;
        bool finish;
    };

    struct InputSource
    {
        std::string scd_path;
        bool position;
    };




    

    
public:

    ScdMerger()
    {
    }

    ScdMerger(const std::string& output_dir, bool i_only = true)
    {
        //use the default config
        PropertyConfig config;
        config.output_dir = output_dir;
        config.property_name = "DOCID";
        config.merge_function = &DefaultMergeFunction;
        config.output_function = GetDefaultOutputFunction(i_only);
        config.output_if_no_position = true;
        property_config_.push_back(config);
    }

    void AddPropertyConfig(const PropertyConfig& config)
    {
        property_config_.push_back(config);
    }


    void AddInput(const InputSource& is)
    {
        input_list_.push_back(is);
    }

    void AddInput(const std::string& scd_path, bool position = true)
    {
        InputSource is;
        is.scd_path = scd_path;
        is.position = position;
        AddInput(is);
    }
    
    void Output()
    {
        namespace bfs = boost::filesystem;
        LOG(INFO)<<"Start merging... "<<std::endl;
        if(input_list_.empty())
        {
            LOG(WARNING)<<"input list empty"<<std::endl;
            return;
        }
        //build PropertyConfig index
        typedef std::vector<PropertyConfigR> PropertyConfigList;
        typedef boost::unordered_map<std::string, PropertyConfigR*> PropertyConfigIndex;
        PropertyConfigList property_config_list;
        PropertyConfigIndex property_config_index;
        for(uint32_t i=0;i<property_config_.size();i++)
        {
            property_config_list.push_back(PropertyConfigR(property_config_[i]));
        }
        for(uint32_t i=0;i<property_config_list.size();i++)
        {
            property_config_index[property_config_list[i].property_name] = &property_config_list[i];
        }
        uint32_t p = 0;
        for (uint32_t i=0;i<input_list_.size();i++)
        {
            std::vector<std::string> scd_list;
            const InputSource& is = input_list_[i];
            if(!is.position) continue;
            ScdParser::getScdList(is.scd_path, scd_list);
            if(scd_list.empty())
            {
                LOG(WARNING)<<"no scd under "<<is.scd_path<<std::endl;
                continue;
            }
            for(uint32_t s=0;s<scd_list.size();s++)
            {
                std::string scd_file = scd_list[s];
                ScdParser parser(izenelib::util::UString::UTF_8);
                LOG(INFO)<<"Processing "<<scd_file<<std::endl;
                //int type = ScdParser::checkSCDType(scd_file);
                parser.load(scd_file);
                for (ScdParser::iterator doc_iter = parser.begin(); doc_iter != parser.end(); ++doc_iter)
                {
                    ++p;
                    if(p%100000==0)
                    {
                        LOG(INFO)<<"Process "<<(std::size_t)p<<" position scd docs"<<std::endl; 
                    }
                    SCDDoc& doc = *(*doc_iter);
                    std::vector<std::pair<std::string, izenelib::util::UString> >::iterator pit;
                    std::string sdocid;
                    for (pit = doc.begin(); pit != doc.end(); pit++)
                    {
                        const std::string& property_name = pit->first;
                        PropertyConfigIndex::iterator cit = property_config_index.find(property_name);
                        if(cit!=property_config_index.end())
                        {
                            IdType id = GenId_(pit->second);
                            //cit->second->position_map[id] = p;
                            PositionType pp = cit->second->position_map->getInteger(id);
                            uint32_t p1=0, p2=0;
                            if(pp==(PositionType)izenelib::am::succinct::fujimap::NOTFOUND)
                            {
                                pp = CombinePosition_(p,p);
                                cit->second->position_map->setInteger(id, pp, true);
                            }
                            else
                            {
                                SplitPosition_(pp, p1, p2);
                                pp = CombinePosition_(p1, p);
                                cit->second->position_map->setInteger(id, pp, true);

                            }
                            std::string sss;
                            pit->second.convertString(sss, izenelib::util::UString::UTF_8);
                            if(sss=="1f6a088f1f6e605c459fbfdd6df612e5" && property_name=="uuid")
                            {
                                LOG(INFO)<<property_name<<","<<sss<<","<<p<<","<<p1<<","<<p2<<","<<pp<<std::endl;
                            }
                        }
                    }
                }
            }
        }
        std::string cache_dir;
        for(PropertyConfigList::iterator pci_it = property_config_list.begin(); pci_it!=property_config_list.end();++pci_it)
        {
            PropertyConfigR& config = *pci_it;
            LOG(INFO)<<"building fujimap for "<<config.property_name<<std::endl;
            if(config.position_map->build()<0)
            {
                LOG(ERROR)<<config.property_name<<" fujimap build error"<<std::endl;
            }
            std::string output_dir = config.output_dir;
            cache_dir = output_dir;
            config.i_writer.reset(new ScdWriter(output_dir, INSERT_SCD));
            config.u_writer.reset(new ScdWriter(output_dir, UPDATE_SCD));
            config.d_writer.reset(new ScdWriter(output_dir, DELETE_SCD));
        }
        while(true)
        {
            bool all_finish = true;
            for(PropertyConfigList::iterator pci_it = property_config_list.begin(); pci_it!=property_config_list.end();++pci_it)
            {
                if(!pci_it->finish)
                {
                    all_finish = false;
                    break;
                }
            }
            if(all_finish) break;
            std::set<std::string> unfinished;
            Cache cache_all(cache_dir);
            const uint32_t max_cache_size = 1000000;
            //uint32_t cache_size = 0;

            p = 0; //reset
            uint32_t n=0;
            for (uint32_t i=0;i<input_list_.size();i++)
            {
                std::vector<std::string> scd_list;
                const InputSource& is = input_list_[i];
                ScdParser::getScdList(is.scd_path, scd_list);
                if(scd_list.empty())
                {
                    LOG(WARNING)<<"no scd under "<<is.scd_path<<std::endl;
                    continue;
                }
                for(uint32_t s=0;s<scd_list.size();s++)
                {
                    std::string scd_file = scd_list[s];
                    ScdParser parser(izenelib::util::UString::UTF_8);
                    LOG(INFO)<<"Processing "<<scd_file<<std::endl;
                    int type = ScdParser::checkSCDType(scd_file);
                    parser.load(scd_file);
                    for (ScdParser::iterator doc_iter = parser.begin(); doc_iter != parser.end(); ++doc_iter)
                    {
                        n++;
                        if(n%100000==0)
                        {
                            LOG(INFO)<<"Process "<<n<<" scd docs"<<std::endl; 
                        }
                        uint32_t position = 0;
                        if(is.position) 
                        {
                            ++p;
                            position = p;
                        }
                        SCDDoc& scd_doc = *(*doc_iter);
                        Document doc = getDoc_(scd_doc);
                        ValueType output_doc_value;
                        for(PropertyConfigList::iterator pci_it = property_config_list.begin(); pci_it!=property_config_list.end();++pci_it)
                        {
                            if(pci_it->finish) continue;
                            std::size_t in_mem_num = cache_all.in_mem_num();
                            if(n%100000==0)
                            {
                                LOG(INFO)<<"mem cache size "<<in_mem_num<<std::endl; 
                            }
                            if(in_mem_num==max_cache_size)
                            {
                                //cache full
                                cache_all.set_cache_full();
                            }
                            const std::string& pname = pci_it->property_name;
                            PropertyConfigR& config = *pci_it;
                            ValueType value(doc, type);
                            if(pname!="DOCID")
                            {
                                value = output_doc_value;
                            }
                            izenelib::util::UString pvalue;
                            if(!value.doc.getProperty(pname, pvalue)) continue;
                            if(pvalue.empty()) continue;
                            CacheType& cache = cache_all[pname];
                            ValueType output_value;
                            IdType id = GenId_(pvalue);
                            //ValueType empty_value;
                            //config.merge_function(empty_value, value);
                            //empty_value.swap(value);
                            int pstatus = -1;//-1 means not in position, 0 means not output, 1 means output with cache, 2 means output directly without cache
                            PositionType pos = config.position_map->getInteger(id);
                            if(pos==(PositionType)izenelib::am::succinct::fujimap::NOTFOUND)
                            {
                                pstatus = -1;
                            }
                            else
                            {
                                pstatus = 0;
                                uint32_t p1, p2;
                                SplitPosition_(pos, p1, p2);
                                //LOG(INFO)<<"p1p2 "<<p1<<","<<p2<<std::endl;
                                if(p1==position)
                                {
                                    if(p2==position)
                                    {
                                        pstatus = 3;
                                    }
                                    else
                                    {
                                        pstatus = 2;
                                    }
                                }
                                else
                                {
                                    if(p2==position)
                                    {
                                        pstatus = 1;
                                    }
                                }
                                std::string sss;
                                pvalue.convertString(sss, izenelib::util::UString::UTF_8);
                                if(sss=="1f6a088f1f6e605c459fbfdd6df612e5" && pname=="uuid")
                                {
                                    LOG(INFO)<<pname<<","<<sss<<" "<<p1<<","<<p2<<std::endl;
                                }
                            }
                            //std::cout<<"pstatus "<<pstatus<<","<<pname<<std::endl;
                            std::string sss;
                            pvalue.convertString(sss, izenelib::util::UString::UTF_8);
                            if(sss=="1f6a088f1f6e605c459fbfdd6df612e5" && pname=="uuid")
                            {
                                LOG(INFO)<<pname<<","<<sss<<" "<<pstatus<<std::endl;
                            }
                            if(pstatus<0)
                            {
                                if(pci_it->output_if_no_position)
                                {
                                    output_value = value;
                                }
                                else
                                {
                                    continue;
                                }
                            }
                            else if(pstatus==3)//first and last appear
                            {
                                ValueType empty_value;
                                config.merge_function(empty_value, value);
                                empty_value.swap(value);
                                output_value = value;
                            }
                            else
                            {
                                cache_iterator cit = cache.end();
                                if(pstatus!=2) //not first appear
                                {
                                    cit = cache.find(id);
                                }
                                //ValueType init_value;
                                //ValueType* p_output = NULL;
                                if(cit==cache.end())
                                {
#ifdef MERGER_DEBUG
                                    std::string sss;
                                    pvalue.convertString(sss, izenelib::util::UString::UTF_8);
                                    if(sss=="403ec13d9939290d24c308b3da250658" && pname=="uuid")
                                    {
                                        LOG(INFO)<<pname<<","<<sss<<" not in cache"<<std::endl;
                                    }
#endif
                                    ValueType empty_value;
                                    config.merge_function(empty_value, value);
                                    empty_value.swap(value);
                                }
                                else
                                {
#ifdef MERGER_DEBUG
                                    std::string sss;
                                    pvalue.convertString(sss, izenelib::util::UString::UTF_8);
                                    if(sss=="403ec13d9939290d24c308b3da250658" && pname=="uuid")
                                    {
                                        LOG(INFO)<<pname<<","<<sss<<" merge"<<std::endl;
                                    }
#endif
                                    config.merge_function(cit->second, value);
                                    if(pstatus!=1)
                                    {
                                        cache.update_back(cit);//not last app
                                    }
                                    value = cit->second;
                                }
                                if(pstatus==1)//is last
                                {
#ifdef MERGER_DEBUG
                                    std::string sss;
                                    pvalue.convertString(sss, izenelib::util::UString::UTF_8);
                                    if(sss=="403ec13d9939290d24c308b3da250658" && pname=="uuid")
                                    {
                                        LOG(INFO)<<pname<<","<<sss<<" output"<<std::endl;
                                    }
#endif
                                    output_value = value;
                                }
                                if(!output_value.empty() && cit!=cache.end())
                                {
                                    //LOG(INFO)<<"cache erase "<<std::endl;
                                    cache.erase(cit);
                                }
                                if(output_value.empty() && cit==cache.end())
                                {
#ifdef MERGER_DEBUG
                                    std::string sss;
                                    pvalue.convertString(sss, izenelib::util::UString::UTF_8);
                                    if(sss=="403ec13d9939290d24c308b3da250658" && pname=="uuid")
                                    {
                                        LOG(INFO)<<pname<<","<<sss<<" insert to cache"<<std::endl;
                                    }
#endif
                                    if(in_mem_num<max_cache_size && pstatus==2)//is first appear
                                    {
                                        cache.insert_mem(std::make_pair(id, value));
                                    }
                                    else
                                    {
                                        cache.insert(std::make_pair(id, value));
                                    }
                                }
                            }
                            if(!output_value.empty())
                            {
                                if(pname=="DOCID")
                                {
                                    output_doc_value = output_value;
                                }
                                //output value
                                config.output_function(output_value.doc, output_value.type);
                                //processor->Process(output_value.doc, output_value.type);
                                int type = output_value.type;
                                const Document& document = output_value.doc;
                                if(type==INSERT_SCD)
                                {
                                    config.i_writer->Append(document);
                                }
                                else if(type==UPDATE_SCD)
                                {
                                    config.u_writer->Append(document);
                                }
                                else if(type==DELETE_SCD)
                                {
                                    config.d_writer->Append(document);
                                }
                            }
                        }

                    }
                }
            }

            for(PropertyConfigList::iterator pci_it = property_config_list.begin(); pci_it!=property_config_list.end();++pci_it)
            {
                PropertyConfigR& config = *pci_it;
                config.finish = true;
                if(unfinished.find(config.property_name)!=unfinished.end())
                {
                    config.finish = false;
                }
            }
        }
        
        for(PropertyConfigList::iterator pci_it = property_config_list.begin(); pci_it!=property_config_list.end();++pci_it)
        {
            PropertyConfigR& config = *pci_it;
            config.i_writer->Close();
            config.u_writer->Close();
            config.d_writer->Close();
        }
        
    }
    
    static void DefaultMergeFunction(ValueType& value, const ValueType& another_value)
    {
        if(another_value.type==DELETE_SCD)
        {
            //doc.clear();
            //doc.property("DOCID") = value.doc.property("DOCID");
            value.doc.copyPropertiesFromDocument(another_value.doc, true);
            value.type = another_value.type;
        }
        else if(another_value.type==UPDATE_SCD)
        {
            value.doc.copyPropertiesFromDocument(another_value.doc, true);
            if(value.type!=INSERT_SCD)
            {
                value.type = another_value.type;
            }
        }
        else if(another_value.type==INSERT_SCD)
        {
            value.doc = another_value.doc;
            value.type = another_value.type;
        }
    }

    
    static void DefaultOutputFunction(Document& doc, int& type, bool i_only)
    {
        //LOG(INFO)<<"outputing type: "<<type<<std::endl;
        if(type==UPDATE_SCD)
        {
            if(i_only)
            {
                type = INSERT_SCD;
            }
        }
        else if(type==DELETE_SCD)
        {
            if(i_only)
            {
                type = NOT_SCD;
            }
            else
            {
                doc.clearExceptDOCID();
            }
        }
    }

    static OutputFunctionType GetDefaultOutputFunction(bool i_only)
    {
        return boost::bind(&DefaultOutputFunction, _1, _2, i_only);
    }

private:

    static uint64_t CombinePosition_(uint32_t p1, uint32_t p2) 
    {
        uint64_t r = p1;
        r = r<<32;
        r += p2;
        return r;
    }

    static void SplitPosition_(uint64_t pp, uint32_t& p1, uint32_t& p2)
    {
        p2 = (uint32_t)pp;
        p1 = pp>>32;
    }

    Document getDoc_(SCDDoc& doc)
    {
        Document result;
        SCDDoc::iterator p = doc.begin();
        for (; p != doc.end(); p++)
        {
            result.property(p->first) = p->second;
        }
        return result;
    }

    IdType GenId_(const izenelib::util::UString& id_str)
    {
        return Utilities::md5ToUint128(id_str);
        //return izenelib::util::HashFunction<izenelib::util::UString>::generateHash64(id_str);
    }

    
private:
    std::vector<PropertyConfig> property_config_;
    std::vector<InputSource> input_list_;
    
};

}

//int main(int argc, char** argv)
//{
    //std::string scdPath = argv[1];
    //std::string properties = argv[2];
    //std::string output_dir = argv[3];
    //bool i_only = true;
    //if(argc>=5)
    //{
        //std::string ionlyp = argv[4];
        //if(ionlyp == "--gen-all")
        //{
            //i_only = false;
        //}
    //}
    //std::cout<<"argc : "<<argc<<std::endl;
    //std::vector<std::string> p_vector;
    //boost::algorithm::split( p_vector, properties, boost::algorithm::is_any_of(",") );
    //ScdMerger merger("./scd_merger_workdir", p_vector);
    //merger.Merge(scdPath, output_dir, i_only);
    //bfs::remove_all("./scd_merger_workdir");
//}



