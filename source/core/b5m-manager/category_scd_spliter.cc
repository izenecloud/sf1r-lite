#include "category_scd_spliter.h"
#include "b5m_types.h"
#include "b5m_helper.h"
#include <common/ScdParser.h>
#include <common/ScdWriter.h>
#include <product-manager/product_price.h>
#include <am/sequence_file/ssfr.h>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include <glog/logging.h>

using namespace sf1r;

CategoryScdSpliter::CategoryScdSpliter()
{
}

CategoryScdSpliter::~CategoryScdSpliter()
{
    for(uint32_t i=0;i<values_.size();i++)
    {
        delete values_[i].writer;
    }
}

bool CategoryScdSpliter::Load_(const std::string& category_dir, const std::string& name)
{
    LOG(INFO)<<"Loading "<<category_dir<<std::endl;
    if(!boost::filesystem::exists(category_dir)) return false;
    std::string scd_file = category_dir+"/"+name+".SCD";
    if(boost::filesystem::exists(scd_file)) return true;
    std::string type = "";
    {
        std::string file = category_dir+"/type";
        std::ifstream ifs(file.c_str());
        std::string line;
        if( getline(ifs, line) )
        {
            boost::algorithm::trim(line);
            type = line;
        }
        ifs.close();
    }
    if(type!="attribute") return true;
    {
        std::string regex_file = category_dir+"/category";
        std::ifstream ifs(regex_file.c_str());
        std::string line;
        if( getline(ifs, line) )
        {
            boost::algorithm::trim(line);
            boost::regex r(line);
            ValueType value;
            value.regex = r;
            ScdWriter* writer = new ScdWriter(category_dir, INSERT_SCD);
            writer->SetFileName(name+".SCD");
            value.writer = writer;
            values_.push_back(value);
        }
        ifs.close();
    }
    return true;
}

bool CategoryScdSpliter::Load(const std::string& dir, const std::string& name)
{
    namespace bfs = boost::filesystem;
    if(!bfs::exists(dir)) return false;
    std::string category_file = dir+"/category";
    if(bfs::exists(category_file))
    {
        Load_(dir, name);
    }
    else
    {
        bfs::path p(dir);
        bfs::directory_iterator end;
        for(bfs::directory_iterator it(p);it!=end;it++)
        {
            if(bfs::is_directory(it->path()))
            {
                Load_(it->path().string(), name);
            }
        }
    }
    return true;
}

bool CategoryScdSpliter::Split(const std::string& scd_path)
{
    typedef izenelib::util::UString UString;
    namespace bfs = boost::filesystem;
    if(!bfs::exists(scd_path)) return false;
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

    for(uint32_t i=0;i<scd_list.size();i++)
    {
        std::string scd_file = scd_list[i];
        LOG(INFO)<<"Spliting "<<scd_file<<std::endl;
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
            std::string scategory;
            doc.property("Category").get<UString>().convertString(scategory, UString::UTF_8);
            for(uint32_t i=0;i<values_.size();i++)
            {
                if(boost::regex_match(scategory, values_[i].regex))
                {
                    values_[i].writer->Append(doc);
                    break;
                }
            }
        }
    }
    for(uint32_t i=0;i<values_.size();i++)
    {
        values_[i].writer->Close();
    }
    return true;
}

