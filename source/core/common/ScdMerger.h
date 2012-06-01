#include <common/ScdParser.h>
#include <common/ScdWriter.h>
#include <common/ScdWriterController.h>
#include <document-manager/Document.h>
#include <iostream>
#include <string>

#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include <am/luxio/BTree.h>
#include <am/tc/BTree.h>
#include <am/leveldb/Table.h>
#include <am/range/AmIterator.h>
#include <am/sequence_file/ssfr.h>
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

        //ValueType& operator+=(const ValueType& value)
        //{
            //if(value.type==DELETE_SCD)
            //{
                ////doc.clear();
                ////doc.property("DOCID") = value.doc.property("DOCID");
                //doc.copyPropertiesFromDocument(value.doc, true);
                //type = value.type;
            //}
            //else if(value.type==UPDATE_SCD)
            //{
                //type = value.type;
            //}
            //else if(value.type==INSERT_SCD)
            //{
                //doc = value.doc;
                //type = value.type;
            //}

            //return *this;
        //}
    };
    typedef boost::function<void (ValueType&, const ValueType&) > MergeFunctionType;
    typedef boost::function<void (Document&, int&) > OutputFunctionType;

    typedef uint64_t IdType;
    typedef uint32_t PositionType;

    struct PropertyConfig
    {
        std::string output_dir;
        std::string property_name;
        MergeFunctionType merge_function;
        OutputFunctionType output_function;
        bool output_if_no_position;
    };
    typedef boost::unordered_map<IdType, ValueType> CacheType;
    typedef CacheType::iterator cache_iterator;
    typedef boost::unordered_map<IdType, PositionType> PositionMap;

    struct PropertyConfigR
    {
        PropertyConfigR(const PropertyConfig& config)
        :output_dir(config.output_dir), property_name(config.property_name), merge_function(config.merge_function), output_function(config.output_function), output_if_no_position(config.output_if_no_position)
        {
        }
        std::string output_dir;
        std::string property_name;
        MergeFunctionType merge_function;
        OutputFunctionType output_function;
        bool output_if_no_position;
        boost::shared_ptr<ScdWriter> i_writer;
        boost::shared_ptr<ScdWriter> u_writer;
        boost::shared_ptr<ScdWriter> d_writer;
        PositionMap position_map;
        CacheType cache;
    };

    struct InputSource
    {
        std::string scd_path;
        bool position;
        //bool output_if_no_position;
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
        PositionType p = 0;
        //typedef std::map<std::string, PositionMap> MapType; //key is property name
        //MapType map;
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
                        LOG(INFO)<<"Process "<<p<<" position scd docs"<<std::endl; 
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
                            cit->second->position_map[id] = p;
#ifdef MERGER_DEBUG
                            std::string sss;
                            pit->second.convertString(sss, izenelib::util::UString::UTF_8);
                            if(sss=="403ec13d9939290d24c308b3da250658" && property_name=="uuid")
                            {
                                LOG(INFO)<<property_name<<","<<sss<<","<<p<<std::endl;
                            }
#endif
                        }
                    }
                }
            }
        }
        for(PropertyConfigList::iterator pci_it = property_config_list.begin(); pci_it!=property_config_list.end();++pci_it)
        {
            PropertyConfigR& config = *pci_it;
            std::string output_dir = config.output_dir;
            if(!boost::filesystem::exists(output_dir))
            {
                boost::filesystem::create_directories(output_dir);
            }
            config.i_writer.reset(new ScdWriter(output_dir, INSERT_SCD));
            config.u_writer.reset(new ScdWriter(output_dir, UPDATE_SCD));
            config.d_writer.reset(new ScdWriter(output_dir, DELETE_SCD));
        }
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
                    PositionType position = 0;
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
                        ValueType output_value;
                        IdType id = GenId_(pvalue);
                        //ValueType empty_value;
                        //config.merge_function(empty_value, value);
                        //empty_value.swap(value);
                        PositionMap& position_map = config.position_map;
                        PositionMap::iterator mit = position_map.find(id);
                        if(mit==position_map.end())
                        {
#ifdef MERGER_DEBUG
                            std::string sss;
                            pvalue.convertString(sss, izenelib::util::UString::UTF_8);
                            if(sss=="403ec13d9939290d24c308b3da250658" && pname=="uuid")
                            {
                                LOG(INFO)<<pname<<","<<sss<<" position_map end"<<std::endl;
                            }
#endif
                            if(pci_it->output_if_no_position)
                            {
                                output_value = value;
                            }
                            else
                            {
                                continue;
                            }
                        }
                        else
                        {
                            cache_iterator cit = config.cache.find(id);
                            //ValueType init_value;
                            //ValueType* p_output = NULL;
                            if(cit==config.cache.end())
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
                                value = cit->second;
                            }
                            if(mit->second==position)
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
                                position_map.erase(mit);
                            }
                            if(!output_value.empty() && cit!=config.cache.end())
                            {
                                config.cache.erase(cit);
                            }
                            if(output_value.empty() && cit==config.cache.end())
                            {
#ifdef MERGER_DEBUG
                                std::string sss;
                                pvalue.convertString(sss, izenelib::util::UString::UTF_8);
                                if(sss=="403ec13d9939290d24c308b3da250658" && pname=="uuid")
                                {
                                    LOG(INFO)<<pname<<","<<sss<<" insert to cache"<<std::endl;
                                }
#endif
                                //ValueType empty_value;
                                //config.merge_function(empty_value, value);
                                //empty_value.swap(value);
                                config.cache.insert(std::make_pair(id, value));
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
        return izenelib::util::HashFunction<izenelib::util::UString>::generateHash64(id_str);

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



