#include "scd_generator.h"
#include "b5m_types.h"
#include "b5m_helper.h"
#include <common/ScdParser.h>
#include <common/ScdWriter.h>
#include <common/Utilities.h>
#include <product-manager/product_price.h>
#include <product-manager/uuid_generator.h>
#include <am/sequence_file/ssfr.h>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include <glog/logging.h>

using namespace sf1r;

ScdGenerator::ScdGenerator()
:exclude_(false)
{
}

bool ScdGenerator::Load_(const std::string& category_dir)
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

bool ScdGenerator::Load(const std::string& dir)
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

bool ScdGenerator::Generate(const std::string& scd_path, const std::string& output_dir, const std::string& work_dir)
{
    if(o2p_map_.empty())
    {
        std::cout<<"Mapping is empty, ignore"<<std::endl;
        return true;
    }
    typedef izenelib::util::UString UString;
    std::string working_dir = work_dir;
    if(working_dir.empty())
    {
        working_dir = "./b5m_scd_generator_working";
    }
    namespace bfs = boost::filesystem;
    bfs::remove_all(working_dir);
    bfs::create_directories(working_dir);
    if(!bfs::exists(scd_path)) return false;
    std::string b5mo_dir = output_dir+"/b5mo";
    std::string b5mp_dir = output_dir+"/b5mp";
    bfs::create_directories(b5mo_dir);
    bfs::create_directories(b5mp_dir);

    std::vector<std::string> scd_list;
    if( bfs::is_regular_file(scd_path) && boost::algorithm::ends_with(scd_path, ".SCD"))
    {
        scd_list.push_back(scd_path);
    }
    else if(bfs::is_directory(scd_path))
    {
        bfs::path p(scd_path);
        bfs::directory_iterator end;
        for(bfs::directory_iterator it(p);it!=end;it++)
        {
            if(bfs::is_regular_file(it->path()))
            {
                std::string file = it->path().string();
                if(boost::algorithm::ends_with(file, ".SCD"))
                {
                    scd_list.push_back(file);
                }
            }
        }
    }
    if(scd_list.empty()) return false;

    std::string writer_file = working_dir+"/docs";
    izenelib::am::ssf::Writer<> writer(writer_file);
    writer.Open();

    for(uint32_t i=0;i<scd_list.size();i++)
    {
        std::string scd_file = scd_list[i];
        LOG(INFO)<<"Processing "<<scd_file<<std::endl;
        ScdParser parser(izenelib::util::UString::UTF_8);
        parser.load(scd_file);
        uint32_t n=0;
        for( ScdParser::iterator doc_iter = parser.begin(B5MHelper::B5MO_PROPERTY_LIST.value);
          doc_iter!= parser.end(); ++doc_iter, ++n)
        {
            if(n%10000==0)
            {
                LOG(INFO)<<"Find Documents "<<n<<std::endl;
            }
            Document doc;
            SCDDoc& scddoc = *(*doc_iter);
            SCDDoc::iterator p = scddoc.begin();
            for(; p!=scddoc.end(); ++p)
            {
                const std::string& property_name = p->first;
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
            std::string spid = sdocid;
            boost::unordered_map<std::string, std::string>::iterator it = o2p_map_.find(sdocid);
            if(it!=o2p_map_.end())
            {
                spid = it->second;
            }
            doc.property("uuid") = UString(spid, UString::UTF_8);
            uint64_t hash_id = izenelib::util::HashFunction<std::string>::generateHash64(spid);
            writer.Append(hash_id, doc);
        }
    }
    writer.Close();
    izenelib::am::ssf::Sorter<uint32_t, uint64_t>::Sort(writer_file);
    izenelib::am::ssf::Joiner<uint32_t, uint64_t, Document> joiner(writer_file);
    joiner.Open();
    uint64_t key;
    std::vector<Document> docs;
    ScdWriter b5mo_writer(b5mo_dir, INSERT_SCD);
    ScdWriter b5mp_writer(b5mp_dir, INSERT_SCD);
    uint64_t n = 0;
    while(joiner.Next(key, docs))
    {
        ++n;
        if(n%100000==0)
        {
            LOG(INFO)<<"Process "<<n<<" docs"<<std::endl;
        }
        izenelib::util::UString uuid;
        std::string uuidstr;
        UuidGenerator::Gen(uuidstr, uuid);
        for(uint32_t i=0;i<docs.size();i++)
        {
            Document doc(docs[i]);
            doc.property("uuid") = uuid;
            b5mo_writer.Append(doc);
        }
        Document b5mp_doc;
        bool find = false;
        for(uint32_t i=0;i<docs.size();i++)
        {
            if(docs[i].property("DOCID") == docs[i].property("uuid"))
            {
                b5mp_doc.copyPropertiesFromDocument(docs[i]);
                find = true;
                break;
            }
        }
        if(!find)
        {
            b5mp_doc.copyPropertiesFromDocument(docs[0]);
        }
        b5mp_doc.property("DOCID") = uuid;
        b5mp_doc.eraseProperty("uuid");

        //generate boost::uuid, and push it to log-server
        ProductPrice price;
        std::set<std::string> source_set;
        for(uint32_t i=0;i<docs.size();i++)
        {
            ProductPrice p;
            UString uprice;
            docs[i].getProperty("Price", uprice);
            p.Parse(uprice);
            price += p;
            UString usource;
            docs[i].getProperty("Source", usource);

            if(usource.length()>0)
            {
                std::string source;
                usource.convertString(source, UString::UTF_8);
                source_set.insert(source);
            }
        }
        UString usource;
        std::set<std::string>::iterator it = source_set.begin();
        while(it!=source_set.end())
        {
            if(usource.length()>0)
            {
                usource.append(UString(",", UString::UTF_8));
            }
            usource.append(UString(*it, UString::UTF_8));
            ++it;
        }
        b5mp_doc.property("Price") = price.ToUString();
        b5mp_doc.property("Source") = usource;
        uint32_t itemcount = docs.size();
        b5mp_doc.property("itemcount") = UString(boost::lexical_cast<std::string>(itemcount), UString::UTF_8);
        b5mp_writer.Append(b5mp_doc);
    }
    b5mo_writer.Close();
    b5mp_writer.Close();
    boost::filesystem::remove_all(working_dir);
    return true;
}

