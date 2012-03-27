#include "b5mo_scd_generator.h"
#include "b5m_types.h"
#include "b5m_helper.h"
#include <common/ScdParser.h>
#include <common/ScdWriter.h>
#include <common/ScdWriterController.h>
#include <common/Utilities.h>
#include <product-manager/product_price.h>
#include <product-manager/uuid_generator.h>
#include <am/sequence_file/ssfr.h>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include <glog/logging.h>

using namespace sf1r;

B5MOScdGenerator::B5MOScdGenerator()
:exclude_(false)
{
}

bool B5MOScdGenerator::Load_(const std::string& category_dir)
{
    LOG(INFO)<<"Loading "<<category_dir<<std::endl;
    if(!boost::filesystem::exists(category_dir)) return false;
    {
        std::string regex_file = category_dir+"/category";
        std::ifstream ifs(regex_file.c_str());
        std::string line;
        if( getline(ifs, line) )
        {
            boost::algorithm::trim(line);
            boost::regex r(line);
            category_regex_.push_back(r);
        }
        ifs.close();
    }
    {
        std::string match_file = category_dir+"/match";
        std::ifstream ifs(match_file.c_str());
        std::string line;
        while( getline(ifs,line))
        {
            boost::algorithm::trim(line);
            std::vector<std::string> vec;
            boost::algorithm::split(vec, line, boost::is_any_of(","));
            if(vec.size()<2) continue;
            o2p_map_.insert(std::make_pair(vec[0], vec[1]));
        }
        ifs.close();
    }
    return true;
}

bool B5MOScdGenerator::Load(const std::string& dir)
{
    namespace bfs = boost::filesystem;
    if(!bfs::exists(dir)) return false;
    std::string match_file = dir+"/match";
    std::string category_file = dir+"/category";
    if(bfs::exists(match_file))
    {
        Load_(dir);
    }
    else if(!bfs::exists(category_file))
    {
        bfs::path p(dir);
        bfs::directory_iterator end;
        for(bfs::directory_iterator it(p);it!=end;it++)
        {
            if(bfs::is_directory(it->path()))
            {
                Load_(it->path().string());
            }
        }
    }
    return true;
}

bool B5MOScdGenerator::Generate(const std::string& scd_path, const std::string& output_dir)
{
    if(o2p_map_.empty())
    {
        std::cout<<"Mapping is empty, ignore"<<std::endl;
        return true;
    }
    typedef izenelib::util::UString UString;
    namespace bfs = boost::filesystem;
    bfs::create_directories(output_dir);

    std::vector<std::string> scd_list;
    B5MHelper::GetScdList(scd_path, scd_list);
    if(scd_list.empty()) return false;

    ScdWriterController writer(output_dir);

    for(uint32_t i=0;i<scd_list.size();i++)
    {
        std::string scd_file = scd_list[i];
        LOG(INFO)<<"Processing "<<scd_file<<std::endl;
        int scd_type = ScdParser::checkSCDType(scd_file);
        ScdParser parser(izenelib::util::UString::UTF_8);
        parser.load(scd_file);
        uint32_t n=0;
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
            if(exclude_)
            {
                std::string scategory;
                doc.property("Category").get<UString>().convertString(scategory, UString::UTF_8);
                bool find_match = false;
                for(uint32_t i=0;i<category_regex_.size();i++)
                {
                    if(boost::regex_match(scategory, category_regex_[i]))
                    {
                        find_match = true;
                        break;
                    }
                }
                if(!find_match) continue;
            }
            std::string sdocid;
            doc.property("DOCID").get<UString>().convertString(sdocid, UString::UTF_8);
            //std::string spid = sdocid;
            std::string spid;
            boost::unordered_map<std::string, std::string>::iterator it = o2p_map_.find(sdocid);
            if(it!=o2p_map_.end())
            {
                spid = it->second;
            }
            if(spid.empty() && scd_type == INSERT_SCD)
            {
                spid = sdocid;
            }
            if(!spid.empty())
            {
                doc.property("uuid") = UString(spid, UString::UTF_8);
            }
            writer.Write(doc, scd_type);
        }
    }
    writer.Flush();
    return true;
}

