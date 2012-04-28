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


namespace sf1r
{

class ScdMerger
{
    
    //struct ValueType
    //{
        //Document doc;
        //int type;
        //uint32_t n;
        
        //friend class boost::serialization::access;
        //template<class Archive>
        //void serialize(Archive & ar, const unsigned int version)
        //{
            //ar & doc & type & n;
        //}
        
        //bool operator<(const ValueType& other) const
        //{
            //return n<other.n;
        //}
    //};

    struct ValueType
    {
        Document doc;
        int type;

        ValueType& operator+=(const ValueType& value)
        {
            if(value.type==DELETE_SCD)
            {
                doc.clear();
                doc.property("DOCID") = value.doc.property("DOCID");
                type = value.type;
            }
            else if(value.type==UPDATE_SCD)
            {
                doc.copyPropertiesFromDocument(value.doc, true);
                type = value.type;
            }
            else if(value.type==INSERT_SCD)
            {
                doc = value.doc;
                type = value.type;
            }

            return *this;
        }
    };
    
    typedef boost::unordered_map<std::string, uint32_t> MapType;
    typedef MapType::iterator map_iterator;
    
public:
    ScdMerger(const std::string& work_dir,const std::vector<std::string>& propertyNameList)
    : work_dir_(work_dir), propertyNameList_(propertyNameList)
    {
    }
    
    void Merge(const std::string& scdPath, const std::string& output_dir, bool i_only = true)
    {
        namespace bfs = boost::filesystem;
        bfs::remove_all(work_dir_);
        bfs::create_directories(work_dir_);
        LOG(INFO)<<"Start merging, properties list : "<<std::endl;
        for(uint32_t i=0;i<propertyNameList_.size();i++)
        {
            std::cout<<propertyNameList_[i]<<std::endl;
        }
        std::string working_file = work_dir_+"/working";
        bfs::remove_all(working_file);
        izenelib::am::ssf::Writer<> writer(working_file);
        writer.Open();
        std::vector<std::string> scd_list;
        ScdParser::getScdList(scdPath, scd_list);
        if(scd_list.empty()) return;
        uint32_t n = 0;
        LOG(INFO)<<"Total SCD count : "<<scd_list.size()<<std::endl;
        for (uint32_t i=0;i<scd_list.size();i++)
        {
            std::string scd_file = scd_list[i];
            ScdParser parser(izenelib::util::UString::UTF_8);
            LOG(INFO)<<"Processing "<<scd_file<<std::endl;
            int type = ScdParser::checkSCDType(scd_file);
            parser.load(scd_file);
            for (ScdParser::iterator doc_iter = parser.begin(propertyNameList_); doc_iter != parser.end(); ++doc_iter)
            {
                ++n;
                if(n%100000==0)
                {
                    LOG(INFO)<<"Process "<<n<<" scd docs"<<std::endl; 
                }
                //ValueType value;
                //value.n = n;
                //value.type = type;
                SCDDoc doc = *(*doc_iter);
                std::vector<std::pair<std::string, izenelib::util::UString> >::iterator p;
                std::string sdocid;
                for (p = doc.begin(); p != doc.end(); p++)
                {
                    const std::string& property_name = p->first;
                    if(property_name == "DOCID")
                    {
                        p->second.convertString(sdocid, izenelib::util::UString::UTF_8);
                        break;
                    }
                }
                if(sdocid.empty()) continue;
                //std::cout<<"find docid "<<sdocid<<std::endl;
                map_[sdocid] = n;
            }
        }
        if(!boost::filesystem::exists(output_dir))
        {
            boost::filesystem::create_directories(output_dir);
        }
        ScdWriter i_writer(output_dir, INSERT_SCD);
        ScdWriter u_writer(output_dir, UPDATE_SCD);
        ScdWriter d_writer(output_dir, DELETE_SCD);
        n = 0;
        typedef boost::unordered_map<std::string, ValueType> CacheType;
        typedef CacheType::iterator cache_iterator;
        boost::unordered_map<std::string, ValueType> cache;
        for (uint32_t i=0;i<scd_list.size();i++)
        {
            std::string scd_file = scd_list[i];
            ScdParser parser(izenelib::util::UString::UTF_8);
            std::cout<<"X Processing "<<scd_file<<std::endl;
            int type = ScdParser::checkSCDType(scd_file);
            parser.load(scd_file);
            for (ScdParser::iterator doc_iter = parser.begin(propertyNameList_); doc_iter != parser.end(); ++doc_iter)
            {
                ++n;
                if(n%100000==0)
                {
                    LOG(INFO)<<"Process "<<n<<" scd docs"<<std::endl; 
                }
                //ValueType value;
                //value.n = n;
                //value.type = type;
                SCDDoc scd_doc = *(*doc_iter);
                Document doc = getDoc_(scd_doc);
                std::string sdocid;
                doc.getString("DOCID", sdocid);
                if(sdocid.empty()) continue;
                ValueType value;
                value.doc = doc;
                value.type = type;
                bool output = false;
                map_iterator mit = map_.find(sdocid);
                if(mit==map_.end()) continue;
                if(mit->second==n)
                {
                    output = true;
                    map_.erase(mit);
                }
                cache_iterator cit = cache.find(sdocid);
                if(output)
                {
                    if(cit!=cache.end())
                    {
                        cit->second += value;
                        value = cit->second;
                        cache.erase(cit);
                    }
                    //output value
                    int type = value.type;
                    const Document& document = value.doc;
                    if(type==INSERT_SCD)
                    {
                        i_writer.Append(document);
                    }
                    else if(type==UPDATE_SCD)
                    {
                        if(i_only)
                        {
                            i_writer.Append(document);
                        }
                        else
                        {
                            u_writer.Append(document);
                        }
                    }
                    else if(type==DELETE_SCD)
                    {
                        if(!i_only)
                        {
                            d_writer.Append(document);
                        }
                    }
                }
                else
                {
                    if(cit!=cache.end())
                    {
                        cit->second += value;
                    }
                    else
                    {
                        cache.insert(std::make_pair(sdocid, value));
                    }
                }

            }
        }
        
        
        i_writer.Close();
        u_writer.Close();
        d_writer.Close();
        bfs::remove_all(working_file);
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
    
private:
    std::string work_dir_;
    std::vector<std::string> propertyNameList_;
    boost::unordered_map<std::string, uint32_t> map_;
    
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



