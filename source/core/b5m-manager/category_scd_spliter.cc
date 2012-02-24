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

bool CategoryScdSpliter::Split(const std::string& scd_file)
{
    typedef izenelib::util::UString UString;
    if(!boost::filesystem::exists(scd_file)) return false;

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
    for(uint32_t i=0;i<values_.size();i++)
    {
        values_[i].writer->Close();
    }
    return true;
}

