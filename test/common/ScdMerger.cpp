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
using namespace sf1r;
namespace bfs = boost::filesystem;
using namespace std;



class ScdMerger
{
    
    struct ValueType
    {
        Document doc;
        int type;
        uint32_t n;
        
        friend class boost::serialization::access;
        template<class Archive>
        void serialize(Archive & ar, const unsigned int version)
        {
            ar & doc & type & n;
        }
        
        bool operator<(const ValueType& other) const
        {
            return n<other.n;
        }
    };
    
    
public:
    ScdMerger(const std::string& work_dir,const std::vector<string>& propertyNameList)
    : work_dir_(work_dir), propertyNameList_(propertyNameList)
    {
        bfs::remove_all(work_dir_);
        bfs::create_directories(work_dir_);
    }
    
    void Merge(const std::string& scdPath, const std::string& output_dir, bool i_only = true)
    {
        std::cout<<"Start merging, properties list : "<<std::endl;
        for(uint32_t i=0;i<propertyNameList_.size();i++)
        {
            std::cout<<propertyNameList_[i]<<std::endl;
        }
        std::string working_file = work_dir_+"/working";
        bfs::remove_all(working_file);
        izenelib::am::ssf::Writer<> writer(working_file);
        writer.Open();
        std::vector<std::string> scdList;
        ScdParser parser(izenelib::util::UString::UTF_8);
        static const bfs::directory_iterator kItrEnd;
        for (bfs::directory_iterator itr(scdPath); itr != kItrEnd; ++itr)
        {
            if (bfs::is_regular_file(itr->status()))
            {
                std::string file = itr->path().string();
                if (parser.checkSCDFormat(file))
                {
                    scdList.push_back(file);
                }
            }
        }
        if(scdList.empty()) return;
        std::sort(scdList.begin(), scdList.end(), ScdParser::compareSCD);
        std::vector<std::string>::iterator scd_it;
        std::size_t n = 0;
        std::cout<<"Total SCD count : "<<scdList.size()<<std::endl;
        for (scd_it = scdList.begin(); scd_it != scdList.end(); scd_it++)
        {
            std::cout<<"Processing "<<*scd_it<<std::endl;
            int type = ScdParser::checkSCDType(*scd_it);
            //if(i_only && type!=INSERT_SCD) continue;
            parser.load(*scd_it);
            for (ScdParser::iterator doc_iter = parser.begin(propertyNameList_); doc_iter != parser.end(); ++doc_iter, ++n)
            {
                if(n%1000==0)
                {
                    std::cout<<"## Process "<<n<<" scd docs"<<std::endl; 
                }
                ValueType value;
                value.n = n;
                value.type = type;
                SCDDocPtr doc = (*doc_iter);
                value.doc = getDoc_(*doc);
                izenelib::util::UString docname = value.doc.property("DOCID").get<izenelib::util::UString>();
                if(docname.length()==0) continue;
                
                uint64_t hash = izenelib::util::HashFunction<izenelib::util::UString>::generateHash64(docname);
                
                writer.Append(hash, value);
                std::string str_docname;
                docname.convertString(str_docname, izenelib::util::UString::UTF_8);
            }
        }
        
        writer.Close();
        izenelib::am::ssf::Sorter<uint32_t, uint64_t>::Sort(working_file);
        izenelib::am::ssf::Joiner<uint32_t, uint64_t, ValueType> joiner(working_file);
        joiner.Open();
        if(!boost::filesystem::exists(output_dir))
        {
            boost::filesystem::create_directories(output_dir);
        }
        else
        {
            if(boost::filesystem::is_regular_file(output_dir))
            {
                std::cerr<<output_dir<<" is a file"<<std::endl;
                exit(EXIT_FAILURE);
            }
        }
        ScdWriter i_writer(output_dir, INSERT_SCD);
        ScdWriter u_writer(output_dir, UPDATE_SCD);
        ScdWriter d_writer(output_dir, DELETE_SCD);
        uint64_t key;
        std::vector<ValueType> value_list;
        
        while( joiner.Next(key, value_list) )
        {
            std::sort(value_list.begin(), value_list.end());
            Document document;
            int type = 0;
            for( uint32_t i=0;i<value_list.size();i++)
            {
                ValueType& value = value_list[i];
                if(value.type==DELETE_SCD)
                {
                    document.clear();
                    document.property("DOCID") = value.doc.property("DOCID");
                    type = value.type;
                }
                else if(value.type==UPDATE_SCD)
                {
                    document.copyPropertiesFromDocument(value.doc, true);
                    type = value.type;
                }
                else if(value.type==INSERT_SCD)
                {
                    document = value.doc;
                    type = value.type;
                }
            }
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
    std::vector<string> propertyNameList_;
    
};



int main(int argc, char** argv)
{
    std::string scdPath = argv[1];
    std::string properties = argv[2];
    std::string output_dir = argv[3];
    bool i_only = true;
    if(argc>=5)
    {
        std::string ionlyp = argv[4];
        if(ionlyp == "--gen-all")
        {
            i_only = false;
        }
    }
    std::cout<<"argc : "<<argc<<std::endl;
    std::vector<std::string> p_vector;
    boost::algorithm::split( p_vector, properties, boost::algorithm::is_any_of(",") );
    ScdMerger merger("./scd_merger_workdir", p_vector);
    merger.Merge(scdPath, output_dir, i_only);
    bfs::remove_all("./scd_merger_workdir");
}



