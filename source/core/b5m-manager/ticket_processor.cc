#include "ticket_processor.h"
#include "b5m_types.h"
#include "b5m_helper.h"
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <glog/logging.h>
#include <common/ScdParser.h>
#include <common/ScdWriter.h>
#include <document-manager/Document.h>
#include <mining-manager/util/split_ustr.h>
#include <product-manager/product_term_analyzer.h>

//#define TICKET_DEBUG

using namespace sf1r;

TicketProcessor::TicketProcessor()
{
}

bool TicketProcessor::Generate(const std::string& scd_path, const std::string& mdb_instance)
{
    namespace bfs = boost::filesystem;
    std::vector<std::string> scd_list;
    B5MHelper::GetIUScdList(scd_path, scd_list);
    if(scd_list.empty()) return false;
    typedef boost::unordered_map<std::string, std::string> MatchResult;
    MatchResult match_result;


    {
        ProductTermAnalyzer analyzer(cma_path_);
        std::string work_dir = mdb_instance+"/work_dir";

        boost::filesystem::create_directories(work_dir);
        std::string dd_container = work_dir +"/dd_container";
        std::string group_table_file = work_dir +"/group_table";
        GroupTableType group_table(group_table_file);
        group_table.Load();

        DDType dd(dd_container, &group_table);
        dd.SetFixK(12);
        if(!dd.Open())
        {
            std::cout<<"DD open failed"<<std::endl;
            return false;
        }

        for(uint32_t i=0;i<scd_list.size();i++)
        {
            std::string scd_file = scd_list[i];
            LOG(INFO)<<"DD Processing "<<scd_file<<std::endl;
            ScdParser parser(izenelib::util::UString::UTF_8);
            parser.load(scd_file);
            //int scd_type = ScdParser::checkSCDType(scd_file);
            uint32_t n=0;
            for( ScdParser::iterator doc_iter = parser.begin();
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
                //if(doc["uuid"].length()>0) continue;
                UString category = doc["Category"];
                UString city = doc["PlayCity"];
                UString address = doc["PlayAddress"];
                UString time = doc["PlayTime"];
                UString name = doc["PlayName"];
                if(category.empty()||city.empty()||address.empty()||time.empty()||name.empty())
                {
                    continue;
                }
                std::string scategory;
                category.convertString(scategory, izenelib::util::UString::UTF_8);
                std::string soid;
                doc["DOCID"].convertString(soid, izenelib::util::UString::UTF_8);
                const DocIdType& id = soid;

                TicketProcessorAttach attach;
                UString sid_str = category;
                sid_str.append(UString("|", UString::UTF_8));
                sid_str.append(city);
                attach.sid = izenelib::util::HashFunction<izenelib::util::UString>::generateHash32(sid_str);
                std::string stime;
                time.convertString(stime, UString::UTF_8);
                boost::algorithm::split(attach.time_array, stime, boost::is_any_of(",;"));
                std::sort(attach.time_array.begin(), attach.time_array.end());

                UString text = name;
                //text.append(UString(" ", UString::UTF_8));
                //text.append(address);
                //text.append(UString(" ", UString::UTF_8));
                //text.append(category);
                //text.append(UString(" ", UString::UTF_8));
                //text.append(category);
                //text.append(UString(" ", UString::UTF_8));
                //text.append(category);
                //text.append(UString(" ", UString::UTF_8));
                //text.append(city);
                //text.append(UString(" ", UString::UTF_8));
                //text.append(city);
                //text.append(UString(" ", UString::UTF_8));
                //text.append(city);
                std::vector<std::pair<std::string, double> > doc_vector;
                analyzer.Analyze(text, doc_vector);

                if( doc_vector.empty() )
                {
                    continue;
                }
                dd.InsertDoc(id, doc_vector, attach);
#ifdef TICKET_DEBUG
                LOG(INFO)<<"insert id "<<id<<std::endl;
                for(uint32_t i=0;i<doc_vector.size();i++)
                {
                    LOG(INFO)<<doc_vector[i].first<<","<<doc_vector[i].second<<std::endl;
                }
#endif
            }
        }
        dd.RunDdAnalysis();
        const std::vector<std::vector<std::string> >& group_info = group_table.GetGroupInfo();
        for(uint32_t gid=0;gid<group_info.size();gid++)
        {
            const std::vector<std::string>& in_group = group_info[gid];
            if(in_group.empty()) continue;
            std::string pid = in_group[0];
            for(uint32_t i=0;i<in_group.size();i++)
            {
                match_result[in_group[i]] = pid;
            }
        }
    }
    LOG(INFO)<<"match result size "<<match_result.size()<<std::endl;
    std::string output_dir = B5MHelper::GetB5moPath(mdb_instance);
    B5MHelper::PrepareEmptyDir(output_dir);
    ScdWriter writer(output_dir, UPDATE_SCD);

    for(uint32_t i=0;i<scd_list.size();i++)
    {
        std::string scd_file = scd_list[i];
        LOG(INFO)<<"Processing "<<scd_file<<std::endl;
        ScdParser parser(izenelib::util::UString::UTF_8);
        parser.load(scd_file);
        //int scd_type = ScdParser::checkSCDType(scd_file);
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
            UString oid;
            std::string soid;
            doc.getProperty("DOCID", oid);
            oid.convertString(soid, UString::UTF_8);
            std::string spid;
            MatchResult::const_iterator it = match_result.find(soid);
            if(it==match_result.end())
            {
                spid = soid;
            }
            else
            {
                spid = it->second;
            }
            doc.property("uuid") = UString(spid, UString::UTF_8);
            writer.Append(doc);
        }
    }
    writer.Close();

    return true;

}

