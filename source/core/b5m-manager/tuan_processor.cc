#include "tuan_processor.h"
#include "b5m_types.h"
#include "b5m_helper.h"
#include "product_db.h"
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <glog/logging.h>
#include <common/ScdParser.h>
#include <document-manager/Document.h>
#include <mining-manager/util/split_ustr.h>
#include <product-manager/product_term_analyzer.h>


using namespace sf1r;

TuanProcessor::TuanProcessor()
{
}

bool TuanProcessor::Generate(const std::string& scd_path, const std::string& mdb_instance)
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
        //dd.SetFixK(12);
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
                UString city = doc["City"];
                UString title = doc["Title"];
                if(category.empty()||city.empty()||title.empty())
                {
                    continue;
                }
                std::string scategory;
                category.convertString(scategory, izenelib::util::UString::UTF_8);
                std::string soid;
                doc["DOCID"].convertString(soid, izenelib::util::UString::UTF_8);
                const DocIdType& id = soid;

                TuanProcessorAttach attach;
                UString sid_str = category;
                sid_str.append(UString("|", UString::UTF_8));
                sid_str.append(city);
                attach.sid = izenelib::util::HashFunction<izenelib::util::UString>::generateHash32(sid_str);

                std::vector<std::pair<std::string, double> > doc_vector;
                analyzer.Analyze(title, doc_vector);

                if( doc_vector.empty() )
                {
                    continue;
                }
                dd.InsertDoc(id, doc_vector, attach);
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
    std::string b5mo_path = B5MHelper::GetB5moPath(mdb_instance);
    B5MHelper::PrepareEmptyDir(b5mo_path);
    ScdWriter writer(b5mo_path, UPDATE_SCD);

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
    PairwiseScdMerger merger(b5mo_path);
    std::size_t odoc_count = ScdParser::getScdDocCount(b5mo_path);
    LOG(INFO)<<"tuan o doc count "<<odoc_count<<std::endl;
    uint32_t m = odoc_count/2400000+1;
    merger.SetM(m);
    merger.SetMProperty("uuid");
    merger.SetOutputer(boost::bind( &TuanProcessor::B5moOutput_, this, _1, _2));
    merger.SetMEnd(boost::bind( &TuanProcessor::POutputAll_, this));
    std::string p_output_dir = B5MHelper::GetB5mpPath(mdb_instance);
    B5MHelper::PrepareEmptyDir(p_output_dir);
    pwriter_.reset(new ScdWriter(p_output_dir, UPDATE_SCD));
    merger.Run();
    pwriter_->Close();
    return true;

}

void TuanProcessor::POutputAll_()
{
    for(CacheType::iterator it = cache_.begin();it!=cache_.end();it++)
    {
        SValueType& svalue = it->second;
        svalue.doc.eraseProperty("OID");
        pwriter_->Append(svalue.doc);
    }
    cache_.clear();
}

void TuanProcessor::B5moOutput_(SValueType& value, int status)
{
    uint128_t pid = GetPid_(value.doc);
    if(pid==0)
    {
        return;
    }
    SValueType& svalue = cache_[pid];
    ProductMerge_(svalue, value);
}

uint128_t TuanProcessor::GetPid_(const Document& doc)
{
    std::string spid;
    doc.getString("uuid", spid);
    if(spid.empty()) return 0;
    return B5MHelper::StringToUint128(spid);
}
uint128_t TuanProcessor::GetOid_(const Document& doc)
{
    std::string soid;
    doc.getString("DOCID", soid);
    if(soid.empty()) return 0;
    return B5MHelper::StringToUint128(soid);
}

void TuanProcessor::ProductMerge_(SValueType& value, const SValueType& another_value)
{
    //value is pdoc or empty, another_value is odoc
    ProductProperty pp;
    if(!value.empty())
    {
        pp.Parse(value.doc);
    }
    ProductProperty another;
    another.Parse(another_value.doc);
    pp += another;
    if(value.empty() || another.oid==another.pid )
    {
        value.doc.copyPropertiesFromDocument(another_value.doc, true);
    }
    else
    {
        const PropertyValue& docid_value = value.doc.property("DOCID");
        //override if empty property
        for (Document::property_const_iterator it = another_value.doc.propertyBegin(); it != another_value.doc.propertyEnd(); ++it)
        {
            if (!value.doc.hasProperty(it->first))
            {
                value.doc.property(it->first) = it->second;
            }
            else
            {
                PropertyValue& pvalue = value.doc.property(it->first);
                if(pvalue.which()==docid_value.which()) //is UString
                {
                    const UString& uvalue = pvalue.get<UString>();
                    if(uvalue.empty())
                    {
                        pvalue = it->second;
                    }
                }
            }
        }
    }
    value.type = UPDATE_SCD;
    pp.Set(value.doc);
}

