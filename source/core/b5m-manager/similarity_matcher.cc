#include "similarity_matcher.h"
#include "b5m_types.h"
#include "b5m_helper.h"
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/unordered_set.hpp>
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
    B5MHelper::GetIScdList(scd_path, scd_list);
    if(scd_list.empty()) return false;
    PidDictionary dic;
    dic.Load(dic_path_);
    LOG(INFO)<<"pid dictionary size: "<<dic.Size()<<std::endl;

    std::string work_dir = knowledge_dir+"/work_dir";

    //boost::filesystem::remove_all(work_dir);
    boost::filesystem::create_directories(work_dir);
    std::string dd_container = work_dir +"/dd_container";
    //std::string group_table_file = work_dir +"/group_table";
    //GroupTableType group_table(group_table_file);
    //group_table.Load();
    GroupTableType group_table(&dic);

    DDType dd(dd_container, &group_table);
    dd.SetFixK(3);
    //dd.SetMaxProcessTable(40);
    if(!dd.Open())
    {
        std::cout<<"DD open failed"<<std::endl;
        return false;
    }
    //uint32_t fp_count = dd.GetFpCount();
    //std::cout<<"fp_count : "<<fp_count<<std::endl;
    //uint32_t docid = fp_count+1;
    //std::vector<ValueType> values(1);
    //std::vector<uint32_t> new_id_list;
    //boost::unordered_set<uint32_t> new_id_set;

    for(uint32_t i=0;i<scd_list.size();i++)
    {
        std::string scd_file = scd_list[i];
        LOG(INFO)<<"Processing "<<scd_file<<std::endl;
        ScdParser parser(izenelib::util::UString::UTF_8);
        parser.load(scd_file);
        //int scd_type = ScdParser::checkSCDType(scd_file);
        uint32_t n=0;
        for( ScdParser::iterator doc_iter = parser.begin(B5MHelper::B5MO_PROPERTY_LIST.value);
          doc_iter!= parser.end(); ++doc_iter, ++n)
        {
            if(n%10000==0)
            {
                LOG(INFO)<<"Find Documents "<<n<<std::endl;
            }
            std::map<std::string, UString> doc;
            SCDDoc& scddoc = *(*doc_iter);
            SCDDoc::iterator p = scddoc.begin();
            for(; p!=scddoc.end(); ++p)
            {
                const std::string& property_name = p->first;
                doc[property_name] = p->second;
            }
            if(doc["Category"].length()==0 || doc["Title"].length()==0 || doc["Source"].length()==0)
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
            std::string soid;
            doc["DOCID"].convertString(soid, izenelib::util::UString::UTF_8);
            //DocIdType id = B5MHelper::StringToUint128(soid);
            const DocIdType& id = soid;
            ProductPrice price;
            price.Parse(doc["Price"]);
            if(!price.Valid() || price.value.first<=0.0 )
            {
                continue;
            }
            //if(processed.find(id)!=processed.end()) continue;

            SimilarityMatcherAttach attach;
            attach.category = doc["Category"];
            attach.price = price;
            UString id_str = doc["Source"];
            id_str.append(UString("|", UString::UTF_8));
            id_str.append(doc["Title"]);
            id_str.append(UString("|", UString::UTF_8));
            id_str.append(price.ToUString());
            attach.id = izenelib::util::HashFunction<izenelib::util::UString>::generateHash32(id_str);

            std::vector<std::string> terms;
            std::vector<double> weights;
            analyzer.Analyze(doc["Title"], terms, weights);

            if( terms.empty() )
            {
                continue;
            }
            //dd.InsertDoc(docid, terms, weights, attach);
            dd.InsertDoc(id, terms, weights, attach);
        }
    }
    dd.RunDdAnalysis();
    //LOG(INFO)<<"values size "<<values.size()<<std::endl;
    std::string match_file = knowledge_dir+"/match";
    std::ofstream ofs(match_file.c_str());
    const GroupTableType::ResultType& result = group_table.result;
    for(GroupTableType::ResultType::const_iterator it = result.begin(); it!=result.end();++it)
    {
        boost::posix_time::ptime now = boost::posix_time::second_clock::local_time();
        ofs<<it->first<<","<<it->second.second<<","<<boost::posix_time::to_iso_string(now)<<std::endl;

    }
    LOG(INFO)<<"after dic size: "<<dic.Size()<<std::endl;
    dic.Save(dic_path_);
    //const std::vector<std::vector<std::string> >& group_info = group_table.GetGroupInfo();
    //for(uint32_t gid=0;gid<group_info.size();gid++)
    //{
        //std::vector<std::string> in_group = group_info[gid];
        //std::sort(in_group.begin(), in_group.end());

        //std::string pid_str;
        //if(dic_!=NULL)
        //{
            //for(uint32_t i=0;i<in_group.size();i++)
            //{
                //if(dic_->Exist(in_group[i]))
                //{
                    //pid_str = in_group[i];
                    //break;
                //}
            //}
        //}
        //else
        //{
            //izenelib::util::UString pid_text("groupid-"+boost::lexical_cast<std::string>(gid), izenelib::util::UString::UTF_8);
            //uint128_t pid = izenelib::util::HashFunction<izenelib::util::UString>::generateHash128(pid_text);
            //pid_str = B5MHelper::Uint128ToString(pid);
        //}
        ////for(uint32_t i=0;i<vec.size();i++)
        ////{
            ////std::string soid = vec[i].soid;
            ////std::string stitle;
            ////vec[i].title.convertString(stitle, UString::UTF_8);
            ////boost::posix_time::ptime now = boost::posix_time::second_clock::local_time();
            ////ofs<<soid<<","<<pid_str<<","<<boost::posix_time::to_iso_string(now)<<","<<stitle<<std::endl;
        ////}
        //for(uint32_t i=0;i<in_group.size();i++)
        //{
            //std::string output_pid = pid_str;
            //if(output_pid.empty())
            //{
                //output_pid = in_group[i];//set to itself
            //}
            //boost::posix_time::ptime now = boost::posix_time::second_clock::local_time();
            //ofs<<in_group[i]<<","<<output_pid<<","<<boost::posix_time::to_iso_string(now)<<","<<gid<<std::endl;
        //}
    //}
    ofs.close();

    {
        std::ofstream ofs(done_file.c_str());
        ofs<<"A"<<std::endl;
        ofs.close();
    }
    return true;

}

