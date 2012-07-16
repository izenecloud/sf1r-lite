#include "attribute_processor.h"
#include "b5m_helper.h"
#include <common/ScdParser.h>
#include <common/ScdWriter.h>
#include <document-manager/Document.h>
#include <mining-manager/util/split_ustr.h>
#include <product-manager/product_price.h>
#include <boost/unordered_set.hpp>
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <algorithm>
#include <cmath>
#include <idmlib/util/svm.h>
#include <idmlib/similarity/string_similarity.h>
#include <util/functional.h>
using namespace sf1r;


AttributeProcessor::AttributeProcessor(const std::string& knowledge_dir)
:knowledge_dir_(knowledge_dir), aid_(0), anid_(0)
{
}

AttributeProcessor::~AttributeProcessor()
{
}

//bool AttributeProcessor::LoadSynonym(const std::string& file)
//{
    ////insert to synonym_map_
    //std::ifstream ifs(file.c_str());
    //std::string line;
    //while( getline(ifs, line) )
    //{
        //boost::algorithm::trim(line);
        //if(line.empty()) continue;
        //if(line[0]=='#') continue;
        //boost::to_lower(line);
        //std::vector<std::string> words;
        //boost::algorithm::split( words, line, boost::algorithm::is_any_of(",") );
        //if(words.size()<2) continue;
        //boost::regex r(words[0]);
        //std::string formatter = " "+words[1]+" ";
        //normalize_pattern_.push_back(std::make_pair(r, formatter));
        ////std::string base = words[0];
        ////for(std::size_t i=1;i<words.size();i++)
        ////{
            ////synonym_map_.insert(std::make_pair(words[i], base));
        ////}
    //}
    //ifs.close();
    //return true;
//}

bool AttributeProcessor::Process()
{
    //std::string done_file = knowledge_dir_+"/process.done";
    //if(boost::filesystem::exists(done_file))
    //{
        //std::cout<<knowledge_dir_<<" process done, ignore."<<std::endl;
        //return true;
    //}
    if(!BuildAttributeId_())
    {
        std::cout<<"build aid fail"<<std::endl;
        return false;
    }
    //std::ofstream ofs(done_file.c_str());
    //ofs<<"done"<<std::endl;
    //ofs.close();
    return true;
}



bool AttributeProcessor::BuildAttributeId_()
{
    namespace bfs = boost::filesystem;
    std::string o_scd_dir = knowledge_dir_+"/T";
    std::string t_scd_dir = knowledge_dir_+"/T/P";
    boost::filesystem::remove_all(t_scd_dir);
    boost::filesystem::create_directories(t_scd_dir);
    std::vector<std::string> o_scd_list;
    ScdParser::getScdList(o_scd_dir, o_scd_list);
    if(o_scd_list.empty())
    {
        std::cout<<"o scd empty"<<std::endl;
        return true;
    }
    ScdWriter writer(t_scd_dir, INSERT_SCD);
    for(uint32_t i=0;i<o_scd_list.size();i++)
    {
        ScdParser parser(izenelib::util::UString::UTF_8);
        parser.load(o_scd_list[i]);
        uint32_t n=0;
        for( ScdParser::iterator doc_iter = parser.begin();
          doc_iter!= parser.end(); ++doc_iter, ++n)
        {
            if(n%1000==0)
            {
                LOG(INFO)<<"Find O Documents "<<n<<std::endl;
            }
            Document doc;
            izenelib::util::UString oid;
            izenelib::util::UString title;
            izenelib::util::UString category;
            izenelib::util::UString attrib_ustr;
            SCDDoc& scddoc = *(*doc_iter);
            SCDDoc::iterator p = scddoc.begin();
            for(; p!=scddoc.end(); ++p)
            {
                const std::string& property_name = p->first;
                doc.property(property_name) = p->second;
                if(property_name=="DOCID")
                {
                    oid = p->second;
                }
                else if(property_name=="Title")
                {
                    title = p->second;
                }
                else if(property_name=="Category")
                {
                    category = p->second;
                }
                else if(property_name=="Attribute")
                {
                    attrib_ustr = p->second;
                }
            }
            if(category.length()==0 || attrib_ustr.length()==0 || title.length()==0)
            {
                continue;
            }
            std::string aid_str;
            std::string scategory;
            category.convertString(scategory, izenelib::util::UString::UTF_8);
            std::string stitle;
            title.convertString(stitle, izenelib::util::UString::UTF_8);
            //logger_<<"[BPD][Title]"<<stitle<<std::endl;
            std::vector<AttrPair> attrib_list;
            split_attr_pair(attrib_ustr, attrib_list);
            for(std::size_t i=0;i<attrib_list.size();i++)
            {
                const std::vector<izenelib::util::UString>& attrib_value_list = attrib_list[i].second;
                if(attrib_value_list.size()!=1) continue; //ignore empty value attrib and multi value attribs
                izenelib::util::UString attrib_value = attrib_value_list[0];
                izenelib::util::UString attrib_name = attrib_list[i].first;
                std::string san;
                attrib_name.convertString(san, UString::UTF_8);
                //LOG(INFO)<<"san before "<<san<<std::endl;
                if(boost::algorithm::ends_with(san, "型号"))
                {
                    san = "型号";
                }
                if(boost::algorithm::ends_with(san, "品牌")&&!boost::algorithm::starts_with(san, "品牌"))
                {
                    san = "型号";
                }
                //LOG(INFO)<<"san after "<<san<<std::endl;
                attrib_name = UString(san, UString::UTF_8);
                if(attrib_value.length()==0 || attrib_value.length()>30) continue;
                std::string name_rep = GetAttribRep_(category, attrib_name);
                uint32_t name_id = GetNameId_(category, attrib_name);
                uint32_t aid = GetAid_(category, attrib_name, attrib_value);
                if(!aid_str.empty())
                {
                    aid_str += ",";
                }
                std::string sss = boost::lexical_cast<std::string>(name_id)+":"+boost::lexical_cast<std::string>(aid);
                aid_str += sss;
            }
            doc.property("AID") = UString(aid_str, UString::UTF_8);
            writer.Append(doc);
        }
    }
    writer.Close();
    std::string anid_file = knowledge_dir_+"/T/anid.txt";
    std::string aid_file = knowledge_dir_+"/T/aid.txt";
    std::ofstream anid_ofs(anid_file.c_str());
    std::ofstream aid_ofs(aid_file.c_str());
    for(AidMap::const_iterator it = aid_map_.begin(); it!=aid_map_.end();++it)
    {
        aid_ofs<<it->second<<","<<it->first<<std::endl;
    }
    for(AnidMap::const_iterator it = anid_map_.begin(); it!=anid_map_.end();++it)
    {
        anid_ofs<<it->second<<","<<it->first<<std::endl;
    }
    aid_ofs.close();
    anid_ofs.close();
    return true;

}


uint32_t AttributeProcessor::GetNameId_(const UString& category, const UString& attrib_name)
{
    std::string str = GetAttribRep_(category, attrib_name);
    AnidMap::iterator it = anid_map_.find(str);
    if(it==anid_map_.end())
    {
        ++anid_;
        uint32_t id = anid_;
        anid_map_.insert(std::make_pair(str, id));
        return id;
    }
    else
    {
        return it->second;
    }
}

uint32_t AttributeProcessor::GetAid_(const UString& category, const UString& attrib_name, const UString& attrib_value)
{
    std::string sname;
    attrib_name.convertString(sname, UString::UTF_8);
    std::string svalue;
    attrib_value.convertString(svalue, UString::UTF_8);
    std::vector<std::string> value_vec;
    boost::algorithm::split(value_vec, svalue, boost::algorithm::is_any_of(" "));
    if(value_vec.size()>1 && sname!="型号")
    {
        svalue = value_vec[0];
    }
    boost::algorithm::replace_all(svalue, " ", "");
    std::size_t kpos = svalue.find("(");
    if(kpos!=std::string::npos)
    {
        svalue = svalue.substr(0, kpos);
    }
    kpos = svalue.find("（");
    if(kpos!=std::string::npos)
    {
        svalue = svalue.substr(0, kpos);
    }
    if(true)
    {
        kpos = svalue.find("单机");
        if(kpos!=std::string::npos)
        {
            svalue = svalue.substr(0, kpos);
        }
        kpos = svalue.find("套机");
        if(kpos!=std::string::npos)
        {
            svalue = svalue.substr(0, kpos);
        }
    }
    std::vector<std::string> vec;
    boost::algorithm::split(vec, svalue, boost::algorithm::is_any_of("/"));
    std::vector<UString> values;
    if(vec.size()==2)
    {
        boost::algorithm::to_lower(vec[0]);
        boost::algorithm::to_lower(vec[1]);
        values.push_back(UString(vec[0], UString::UTF_8));
        values.push_back(UString(vec[1], UString::UTF_8));
    }
    else
    {
        boost::algorithm::to_lower(svalue);
        values.push_back(UString(svalue, UString::UTF_8));
    }
    uint32_t result = 0;
    for(uint32_t i=0;i<values.size();i++)
    {
        std::string rep = GetAttribRep_(category, attrib_name, values[i]);
        AidMap::iterator it = aid_map_.find(rep);
        if(it!=aid_map_.end())
        {
            result = it->second;
            break;
        }
    }
    if(result==0)
    {
        ++aid_;
        result = aid_;
    }
    for(uint32_t i=0;i<values.size();i++)
    {
        std::string rep = GetAttribRep_(category, attrib_name, values[i]);
        aid_map_[rep] = result;
    }
    return result;

}


