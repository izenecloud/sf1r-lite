#include "similarity_matcher.h"
#include "b5m_types.h"
#include "b5m_helper.h"
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <am/sequence_file/ssfr.h>
#include <glog/logging.h>
#include <common/ScdParser.h>
#include <common/ScdWriter.h>
#include <document-manager/Document.h>
#include <mining-manager/util/split_ustr.h>
#include <product-manager/product_term_analyzer.h>

using namespace sf1r;

SimilarityMatcher::SimilarityMatcher()
{
}

bool SimilarityMatcher::Index(const std::string& scd_path, const std::string& knowledge_dir)
{
    std::string done_file = knowledge_dir+"/match.done";
    if(boost::filesystem::exists(done_file))
    {
        std::cout<<knowledge_dir<<" match done, ignore."<<std::endl;
        return true;
    }
    ProductTermAnalyzer analyzer(cma_path_);
    CategoryStringMatcher category_matcher;
    std::string category_file = knowledge_dir+"/category";
    if(boost::filesystem::exists(category_file))
    {
        std::ifstream cifs(category_file.c_str());
        std::string line;
        while(getline(cifs, line))
        {
            boost::algorithm::trim(line);
            LOG(INFO)<<"find category regex : "<<line<<std::endl;
            category_matcher.AddCategoryRegex(line);
        }
        cifs.close();
    }

    namespace bfs = boost::filesystem;
    std::vector<std::string> scd_list;
    B5MHelper::GetScdList(scd_path, scd_list);
    if(scd_list.empty()) return false;

    std::string work_dir = knowledge_dir+"/work_dir";

    //boost::filesystem::remove_all(work_dir);
    boost::filesystem::create_directories(work_dir);
    std::string dd_container = work_dir +"/dd_container";
    std::string group_table_file = work_dir +"/group_table";
    GroupTableType group_table(group_table_file);
    group_table.Load();
    DDType dd(dd_container, &group_table);
    dd.SetFixK(3);
    //dd.SetMaxProcessTable(40);
    if(!dd.Open())
    {
        std::cout<<"DD open failed"<<std::endl;
        return false;
    }
    uint32_t docid = 1;
    std::vector<ValueType> values(1);
    std::vector<uint32_t> new_id_list;
    boost::unordered_set<uint32_t> new_id_set;

    for(uint32_t i=0;i<scd_list.size();i++)
    {
        std::string scd_file = scd_list[i];
        LOG(INFO)<<"Processing "<<scd_file<<std::endl;
        ScdParser parser(izenelib::util::UString::UTF_8);
        parser.load(scd_file);
        int scd_type = ScdParser::checkSCDType(scd_file);
        uint32_t n=0;
        for( ScdParser::iterator doc_iter = parser.begin(B5MHelper::B5M_PROPERTY_LIST);
          doc_iter!= parser.end(); ++doc_iter, ++n)
        {
            if(n%10000==0)
            {
                LOG(INFO)<<"Find Documents "<<n<<std::endl;
            }
            izenelib::util::UString oid;
            izenelib::util::UString title;
            izenelib::util::UString category;
            izenelib::util::UString attrib_ustr;
            std::map<std::string, UString> doc;
            SCDDoc& scddoc = *(*doc_iter);
            std::vector<std::pair<izenelib::util::UString, izenelib::util::UString> >::iterator p;
            for(p=scddoc.begin(); p!=scddoc.end(); ++p)
            {
                std::string property_name;
                p->first.convertString(property_name, izenelib::util::UString::UTF_8);
                doc[property_name] = p->second;
            }
            if(doc["Category"].length()==0 || doc["Title"].length()==0)
            {
                continue;
            }
            std::string scategory;
            doc["Category"].convertString(scategory, izenelib::util::UString::UTF_8);
            if( category_matcher.Match(scategory) )
            {
                //std::cout<<"ignore category "<<scategory<<std::endl;
                continue;
            }
            //std::cout<<"valid category "<<scategory<<std::endl;
            std::string stitle;
            title.convertString(stitle, izenelib::util::UString::UTF_8);
            ProductPrice price;
            price.Parse(doc["Price"]);
            if(!price.Valid() || price.value.first<=0.0 )
            {
                continue;
            }

            SimilarityMatcherAttach attach;
            attach.category = doc["Category"];
            attach.price = price;

            std::vector<std::string> terms;
            std::vector<double> weights;
            analyzer.Analyze(doc["Title"], terms, weights);

            if( terms.empty() )
            {
                continue;
            }
            dd.InsertDoc(docid, terms, weights, attach);
            //new_id_list.push_back(docid);
            new_id_set.insert(docid);
            ValueType value;
            doc["DOCID"].convertString(value.soid, UString::UTF_8);
            value.title = doc["Title"];
            values.push_back(value);
            ++docid;
        }
    }
    dd.RunDdAnalysis();
    std::string match_file = knowledge_dir+"/match";
    std::ofstream ofs(match_file.c_str());
    const std::vector<std::vector<uint32_t> >& group_info = group_table.GetGroupInfo();
    for(uint32_t gid=0;gid<group_info.size();gid++)
    {
        const std::vector<uint32_t>& in_group = group_info[gid];
        //if(in_group.size()<2) continue;
        std::vector<uint32_t> new_in_group;
        for(uint32_t i=0;i<in_group.size();i++)
        {
            if(new_id_set.find(in_group[i])!=new_id_set.end())
            {
                new_in_group.push_back(in_group[i]);
            }
        }
        if(new_in_group.empty()) continue;
        std::vector<ValueType> vec(new_in_group.size());
        for(uint32_t i=0;i<new_in_group.size();i++)
        {
            vec[i] = values[new_in_group[i]];
        }
        std::stable_sort(vec.begin(), vec.end());

        izenelib::util::UString pid_text("groupid-"+boost::lexical_cast<std::string>(gid), izenelib::util::UString::UTF_8);
        uint128_t pid = izenelib::util::HashFunction<izenelib::util::UString>::generateHash128(pid_text);
        std::string pid_str = B5MHelper::Uint128ToString(pid);
        //std::string spid = vec[0].soid;
        //std::string sptitle;
        //vec[0].title.convertString(sptitle, UString::UTF_8);
        for(uint32_t i=0;i<vec.size();i++)
        {
            std::string soid = vec[i].soid;
            std::string stitle;
            vec[i].title.convertString(stitle, UString::UTF_8);
            boost::posix_time::ptime now = boost::posix_time::second_clock::local_time();
            ofs<<soid<<","<<pid_str<<","<<boost::posix_time::to_iso_string(now)<<","<<stitle<<std::endl;
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

