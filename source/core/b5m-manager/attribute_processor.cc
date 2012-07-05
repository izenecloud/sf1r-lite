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
using namespace idmlib::sim;

#define Malloc(type,n) (type *)malloc((n)*sizeof(type))

#define B5M_DEBUG

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
    std::string done_file = knowledge_dir_+"/process.done";
    if(boost::filesystem::exists(done_file))
    {
        std::cout<<knowledge_dir<<" process done, ignore."<<std::endl;
        return true;
    }
    if(!BuildAttributeId_())
    {
        std::cout<<"build aid fail"<<std::endl;
        return false;
    }
    std::ofstream ofs(done_file.c_str());
    ofs<<"done"<<std::endl;
    ofs.close();
    return true;
}



bool AttributeProcessor::BuildAttributeId_()
{
    namespace bfs = boost::filesystem;
    std::string o_scd_dir = knowledge_dir_+"/O";
    std::vector<std::string> o_scd_list;
    ScdParser::GetScdList(o_scd_dir, o_scd_list);
    if(o_scd_list.empty())
    {
        std::cout<<"o scd empty"<<std::endl;
        return false;
    }
    std::string t_scd_dir = knowledge_dir_+"/T";
    boost::filesystem::remove_all(t_scd_dir);
    boost::filesystem::create_directories(t_scd_dir);
    ScdWriter writer(t_scd_dir, ScdParser::INSERT_SCD);
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
            for(; p!=doc.end(); ++p)
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
                const izenelib::util::UString& attrib_value = attrib_value_list[0];
                const izenelib::util::UString& attrib_name = attrib_list[i].first;
                if(attrib_value.length()==0 || attrib_value.length()>30) continue;
                std::string name_rep = GetAttribRep(category, attrib_name);
                uint32_t name_id = GetNameId_(category, attrib_name);
                uint32_t aid = GetAid_(category, attrib_name, attrib_value);
                if(!aid_str.empty())
                {
                    aid_str += ",";
                }
                std::string sss = boost::lexical_cast<std::string>(name_id)+":"+boost::lexical_cast<std::string>(aid);
                aid_str += sss;
                std::vector<UString> nav_list;
                NormalizeAV_(attrib_value, nav_list);
                for(uint32_t n=0;n<nav_list.size();n++)
                {
                    UString nav = nav_list[n];
                    //post process nav
                    bool b_value = false;
                    if(IsBoolAttribValue_(nav, b_value))
                    {
                        //if(b_value)
                        //{
                            //nav = attrib_name;
                        //}
                        //else
                        //{
                            //nav.clear();
                        //}
                        //disable bool attrib now
                        nav.clear();
                    }
                    if(nav.length()==0) continue;
                    AttribRep rep = GetAttribRep(category, attrib_name, nav);
                    AttribId aid;
                    id_manager_->getDocIdByDocName(rep, aid);
                    AttribRep name_rep = GetAttribRep(category, attrib_name);
                    AttribNameId name_aid;
                    name_id_manager_->getDocIdByDocName(name_rep, name_aid);
                    name_index_->insert(aid, name_aid);
                    std::string sname_rep;
                    name_rep.convertString(sname_rep, izenelib::util::UString::UTF_8);
                    if(filter_attrib_name_.find(sname_rep)!=filter_attrib_name_.end())
                    {
                        filter_anid_.insert(name_aid);
                    }
                    if(ValidAnid_(name_aid))
                    {
                        product_doc.tag_aid_list.push_back(aid);
                    }
                    std::vector<AttribId> aid_list;
                    index_->get(nav, aid_list);
                    bool aid_dd = false;
                    for(std::size_t j=0;j<aid_list.size();j++)
                    {
                        if(aid==aid_list[j])
                        {
                            aid_dd = true;
                            break;
                        }
                    }
                    if(!aid_dd)
                    {
                        aid_list.push_back(aid);
                        index_->update(nav, aid_list);
                    }
                }

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
    while(AidMap::const_iterator it = aid_map_.begin(); it!=aid_map_.end();++it)
    {
        aid_ofs<<it->second<<","<<it->first<<std::endl;
    }
    while(AnidMap::const_iterator it = anid_map_.begin(); it!=anid_map_.end();++it)
    {
        anid_ofs<<it->second<<","<<it->first<<std::endl;
    }
    aid_ofs.close();
    anid_ofs.close();

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
    std::string svalue;
    attrib_value.convertString(svalue, UString::UTF_8);
    std::size_t index = svalue.find(' ');
    if(index!=std::string::npos)
    {
        svalue = svalue.substr(0, index);
    }
    std::vector<std::string> vec;
    boost::algorithm::split(vec, svalue, boost::algorithm::is_any_of("/"));
    std::vector<UString> values;
    if(vec.size()==2)
    {
        values.push_back(UString(vec[0], UString::UTF_8));
        values.push_back(UString(vec[1], UString::UTF_8));
    }
    else
    {
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


