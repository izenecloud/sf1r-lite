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
    //LOG(INFO)<<"Loading "<<category_dir<<std::endl;
    if(!boost::filesystem::exists(category_dir)) return false;
    std::string scd_dir = category_dir+"/"+name;
    if(boost::filesystem::exists(scd_dir)) return true;
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
            boost::filesystem::create_directories(scd_dir);
            ScdWriter* writer = new ScdWriter(scd_dir, INSERT_SCD);
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
    LOG(INFO)<<"Loading "<<dir<<std::endl;
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
    if(values_.empty())
    {
        LOG(INFO)<<"No target to output split SCD"<<std::endl;
        return true;
    }
    typedef izenelib::util::UString UString;
    namespace bfs = boost::filesystem;
    if(!bfs::exists(scd_path)) return true;
    std::vector<std::string> scd_list;
    B5MHelper::GetIUScdList(scd_path, scd_list);
    if(scd_list.empty()) return true;

    for(uint32_t i=0;i<scd_list.size();i++)
    {
        std::string scd_file = scd_list[i];
        LOG(INFO)<<"Spliting "<<scd_file<<std::endl;
        int scd_type = ScdParser::checkSCDType(scd_list[i]);
        ScdParser parser(izenelib::util::UString::UTF_8);
        parser.load(scd_file);
        uint32_t n=0;
        for( ScdParser::iterator doc_iter = parser.begin();
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
            UString title;
            UString category;
            doc.getProperty("Title", title);
            doc.getProperty("Category", category);
            if( title.length()==0 || category.length()==0) continue;
            UString pid;
            if(doc.getProperty("uuid", pid)) continue;
            std::string scategory;
            category.convertString(scategory, UString::UTF_8);
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

