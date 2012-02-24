#include "complete_matcher.h"
#include "b5m_types.h"
#include "b5m_helper.h"
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <am/sequence_file/ssfr.h>
#include <glog/logging.h>
#include <common/ScdParser.h>
#include <common/ScdWriter.h>
#include <document-manager/Document.h>
#include <mining-manager/util/split_ustr.h>

using namespace sf1r;

CompleteMatcher::CompleteMatcher()
{
}

bool CompleteMatcher::Index(const std::string& scd_file, const std::string& knowledge_dir)
{
    std::string done_file = knowledge_dir+"/match.done";
    if(boost::filesystem::exists(done_file))
    {
        std::cout<<knowledge_dir<<" match done, ignore."<<std::endl;
        return true;
    }
    MatchParameter match_param;
    std::string category_file = knowledge_dir+"/category";
    if(boost::filesystem::exists(category_file))
    {
        std::ifstream cifs(category_file.c_str());
        std::string line;
        if(getline(cifs, line))
        {
            boost::algorithm::trim(line);
            LOG(INFO)<<"find category regex : "<<line<<std::endl;
            match_param.SetCategoryRegex(line);
        }
        cifs.close();
    }
    std::string attrib_name;
    std::string attrib_name_file = knowledge_dir+"/attrib_name";
    if(boost::filesystem::exists(attrib_name_file))
    {
        std::ifstream cifs(attrib_name_file.c_str());
        std::string line;
        if(getline(cifs, line))
        {
            boost::algorithm::trim(line);
            attrib_name = line;
        }
        cifs.close();
    }

    if(attrib_name.empty())
    {
        return false;
    }

    LOG(INFO)<<"find attrib name :"<<attrib_name<<std::endl;
    std::string nattrib_name = attrib_name;
    boost::to_lower(nattrib_name);

    std::string writer_file = knowledge_dir+"/writer";
    boost::filesystem::remove_all(writer_file);
    izenelib::am::ssf::Writer<> writer(writer_file);
    writer.Open();
    ScdParser parser(izenelib::util::UString::UTF_8);
    parser.load(scd_file);
    uint32_t n=0;
    for( ScdParser::iterator doc_iter = parser.begin(B5MHelper::B5M_PROPERTY_LIST);
      doc_iter!= parser.end(); ++doc_iter, ++n)
    {
        if(n%100000==0)
        {
            LOG(INFO)<<"Find Product Documents "<<n<<std::endl;
        }
        ProductDocument product_doc;
        izenelib::util::UString oid;
        izenelib::util::UString title;
        izenelib::util::UString category;
        izenelib::util::UString attrib_ustr;
        SCDDoc& doc = *(*doc_iter);
        std::vector<std::pair<izenelib::util::UString, izenelib::util::UString> >::iterator p;
        for(p=doc.begin(); p!=doc.end(); ++p)
        {
            std::string property_name;
            p->first.convertString(property_name, izenelib::util::UString::UTF_8);
            product_doc.property[property_name] = p->second;
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
        std::string scategory;
        category.convertString(scategory, izenelib::util::UString::UTF_8);
        if( !match_param.MatchCategory(scategory) )
        {
            continue;
        }
        std::string stitle;
        title.convertString(stitle, izenelib::util::UString::UTF_8);
        //logger_<<"[BPD][Title]"<<stitle<<std::endl;
        std::vector<AttrPair> attrib_list;
        split_attr_pair(attrib_ustr, attrib_list);
        UString attrib_for_match;
        for(std::size_t i=0;i<attrib_list.size();i++)
        {
            const std::vector<izenelib::util::UString>& attrib_value_list = attrib_list[i].second;
            if(attrib_value_list.empty()) continue; //ignore empty value attrib
            const izenelib::util::UString& attrib_value = attrib_value_list[0];
            std::string sattrib_name;
            attrib_list[i].first.convertString(sattrib_name, izenelib::util::UString::UTF_8);
            boost::to_lower(sattrib_name);
            boost::algorithm::trim(sattrib_name);
            if(sattrib_name==nattrib_name)
            {
                attrib_for_match = attrib_value;
                break;
            }
        }
        {
            if(attrib_for_match.length()==0) continue;
            std::string sam;
            attrib_for_match.convertString(sam, izenelib::util::UString::UTF_8);
            //std::cout<<"find attrib value : "<<sam<<std::endl;
            boost::algorithm::replace_all(sam, "-", "");
            attrib_for_match = izenelib::util::UString(sam, izenelib::util::UString::UTF_8);
        }
        if(attrib_for_match.length()==0) continue;
        {
            std::string sam;
            attrib_for_match.convertString(sam, izenelib::util::UString::UTF_8);
            //std::cout<<"find attrib value : "<<sam<<std::endl;
        }
        uint64_t hav = izenelib::util::HashFunction<izenelib::util::UString>::generateHash64(attrib_for_match);
        ValueType value;
        std::string soid;
        oid.convertString(soid, UString::UTF_8);
        value.soid = soid;
        value.title = title;
        value.n = n;
        writer.Append(hav, value);
    }
    writer.Close();
    izenelib::am::ssf::Sorter<uint32_t, uint64_t>::Sort(writer_file);
    izenelib::am::ssf::Joiner<uint32_t, uint64_t, ValueType> joiner(writer_file);
    joiner.Open();
    uint64_t hav;
    std::vector<ValueType> value_list;
    std::string match_file = knowledge_dir+"/match";
    std::ofstream ofs(match_file.c_str());
    while(joiner.Next(hav, value_list))
    {
        std::sort(value_list.begin(), value_list.end());
        if(value_list.size()<2) continue;
        ValueType& base = value_list[0];
        std::string sptitle;
        base.title.convertString(sptitle, UString::UTF_8);
        for(uint32_t i=1;i<value_list.size();i++)
        {
            ValueType& value = value_list[i];
            ofs<<value.soid<<","<<base.soid;
            std::string stitle;
            value.title.convertString(stitle, UString::UTF_8);
            ofs<<","<<stitle<<","<<sptitle<<std::endl;
        }
    }
    ofs.close();

    {
        std::ofstream ofs(done_file.c_str());
        ofs<<"A"<<std::endl;
        ofs.close();
    }
    return true;

}

void CompleteMatcher::ClearKnowledge_(const std::string& knowledge_dir)
{
}

