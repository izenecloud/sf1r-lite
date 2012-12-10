#include "psm_indexer.h"
#include <product-manager/product_term_analyzer.h>
#include <util/ClockTimer.h>
#include <util/filesystem.h>
#include <boost/date_time/posix_time/posix_time.hpp>

using namespace sf1r;

PsmIndexer::PsmIndexer(const std::string& cma_path)
:cma_path_(cma_path), psmk_(400)
{
}

bool PsmIndexer::Index(const std::string& scd_path, const std::string& output_path, bool test)
{
    std::string done_file = output_path+"/index.done";
    if(boost::filesystem::exists(done_file))
    {
        LOG(INFO)<<"psm index done "<<done_file<<std::endl;
        return true;
    }
    {
        std::string a_done_file = scd_path+"/index.done";
        if(boost::filesystem::exists(a_done_file))
        {
            LOG(INFO)<<"copy indexed psm knowledge"<<std::endl;
            boost::filesystem::path from(scd_path);
            boost::filesystem::path to(output_path);
            boost::filesystem::remove_all(output_path);
            izenelib::util::filesystem::recursive_copy_directory(from, to);
            return true;
        }
    }
    ProductTermAnalyzer analyzer(cma_path_);
    std::vector<std::string> scd_list;
    B5MHelper::GetIUScdList(scd_path, scd_list);
    if(scd_list.empty())
    {
        LOG(ERROR)<<"scd empty"<<std::endl;
        return false;
    }
    boost::filesystem::create_directories(output_path);
    CategoryStringMatcher cs_matcher(true);//reverse
    std::string category_file = output_path+"/category";
    if(boost::filesystem::exists(category_file))
    {
        std::ifstream cifs(category_file.c_str());
        std::string line;
        while(getline(cifs, line))
        {
            boost::algorithm::trim(line);
            LOG(INFO)<<"find category regex : "<<line<<std::endl;
            cs_matcher.AddCategoryRegex(line);
        }
        cifs.close();
    }

    std::string work_dir = output_path+"/work_dir";
    std::string psm_path = output_path+"/psm";
    B5MHelper::PrepareEmptyDir(work_dir);
    B5MHelper::PrepareEmptyDir(psm_path);
    std::string dd_container = work_dir +"/dd_container";
    std::string group_path = work_dir + "/group_table";
    GroupTableType group_table(group_path);
    group_table.Load();

    DDType dd(dd_container, &group_table);
    dd.SetFixK(3);
    if(!dd.Open())
    {
        std::cout<<"DD open failed"<<std::endl;
        return false;
    }
    std::vector<CacheType> cache_list(1);
    uint32_t cache_index = 0;
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
            std::map<std::string, UString> doc;
            SCDDoc& scddoc = *(*doc_iter);
            SCDDoc::iterator p = scddoc.begin();
            for(; p!=scddoc.end(); ++p)
            {
                const std::string& property_name = p->first;
                doc[property_name] = p->second;
            }
            std::string key;
            std::vector<std::pair<std::string, double> > doc_vector;
            PsmAttach attach;
            if(!PsmHelper::GetPsmItem(cs_matcher, analyzer, doc, key, doc_vector, attach))
            {
                continue;
            }

            cache_index+=1;
            dd.InsertDoc(cache_index, doc_vector, attach);
            cache_list.push_back(CacheType(key, doc_vector, attach));
        }
    }
    dd.RunDdAnalysis();
    const std::vector<std::vector<uint32_t> >& group_info = group_table.GetGroupInfo();
    PsmType psm(psm_path);
    psm.SetK(psmk_);
    psm.Open();
    std::vector<uint32_t> group_centroid(group_info.size(), 0);
    for(uint32_t gid = 0;gid<group_info.size();gid++)
    {
        const std::vector<uint32_t>& in_group = group_info[gid];
        if(in_group.empty()) continue;
        uint32_t ocorrect = 0;
        CacheType ocache;
        uint32_t centroid = 0;
        for(uint32_t i=0;i<in_group.size();i++)
        {
            uint64_t ibits[1] = {0};
            psm.ToFingerprint(cache_list[in_group[i]].doc_vector, ibits);
            uint32_t correct = 0;
            for(uint32_t j=0;j<in_group.size();j++)
            {
                uint64_t jbits[1] = {0};
                psm.ToFingerprint(cache_list[in_group[j]].doc_vector, jbits);
                uint32_t dist = psm.GetDistance(ibits[0], jbits[0]);
                if(dist<=3 && cache_list[in_group[i]].attach.dd(cache_list[in_group[j]].attach))
                {
                    correct+=1;
                }
            }
            if(correct>ocorrect)
            {
                ocorrect = correct;
                centroid = i;
            }
        }
        group_centroid[gid] = centroid;
        uint32_t tdocid = in_group[centroid];
        const CacheType& cache = cache_list[tdocid];
        psm.Insert(cache.key, cache.doc_vector, cache.attach);
    }
    if(!psm.Build())
    {
        LOG(ERROR)<<"psm build error"<<std::endl;
        return false;
    }
    std::ofstream done_ofs(done_file.c_str());
    done_ofs<<"a"<<std::endl;
    done_ofs.close();
    boost::filesystem::remove_all(work_dir);
    if(test)
    {
        uint32_t abit_correct = 0;
        uint32_t acorrect = 0;
        uint32_t aexpect = 0;
        double search_time = 0.0;
        uint32_t search_count = 0;
        izenelib::util::ClockTimer timer;
        for(uint32_t gid = 0;gid<group_info.size();gid++)
        {
            const std::vector<uint32_t>& in_group = group_info[gid];
            if(in_group.size()<2) continue;
            uint32_t centroid = group_centroid[gid];
            LOG(INFO)<<"gid "<<gid<<" in_group size "<<in_group.size()<<", centroid "<<centroid<<std::endl;
            uint32_t odocid = in_group[centroid];
            const CacheType& ocache = cache_list[odocid];
            const std::string& okey = ocache.key;
            uint64_t obits[1] = {0};
            psm.ToFingerprint(ocache.doc_vector, obits);
            uint32_t bit_correct = 0;
            uint32_t correct = 0;
            for(uint32_t i=0;i<in_group.size();i++)
            {
                if(i==centroid) continue;
                const CacheType& cache = cache_list[in_group[i]];
                uint64_t bits[1] = {0};
                psm.ToFingerprint(cache.doc_vector, bits);
                uint32_t dist = psm.GetDistance(obits[0], bits[0]);
                if(dist<=3 && ocache.attach.dd(cache.attach)) bit_correct+=1;
                std::string key;
                timer.restart();
                bool b = psm.Search(cache.doc_vector, cache.attach, key);
                search_time += timer.elapsed();
                search_count += 1;
                if(b&&key==okey)
                {
                    correct+=1;
                }
            }
            aexpect += in_group.size()-1;
            abit_correct += bit_correct;
            acorrect += correct;
            LOG(INFO)<<"stat : "<<bit_correct<<","<<correct<<std::endl;
        }
        LOG(INFO)<<"astat : "<<aexpect<<","<<abit_correct<<","<<acorrect<<std::endl;
        LOG(INFO)<<"time : "<<search_time<<","<<search_count<<std::endl;


        LOG(INFO)<<"real test begin"<<std::endl;
        uint32_t real_count = 0;
        uint32_t real_match = 0;
        for(uint32_t i=0;i<scd_list.size();i++)
        {
            std::string scd_file = scd_list[i];
            LOG(INFO)<<"Testing "<<scd_file<<std::endl;
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
                std::map<std::string, UString> doc;
                SCDDoc& scddoc = *(*doc_iter);
                SCDDoc::iterator p = scddoc.begin();
                for(; p!=scddoc.end(); ++p)
                {
                    const std::string& property_name = p->first;
                    doc[property_name] = p->second;
                }
                std::string key;
                std::vector<std::pair<std::string, double> > doc_vector;
                PsmAttach attach;
                if(!PsmHelper::GetPsmItem(cs_matcher, analyzer, doc, key, doc_vector, attach))
                {
                    continue;
                }
                real_count++;
                bool b = psm.Search(doc_vector, attach, key);
                if(b) real_match++;
            }
        }
        LOG(INFO)<<"real test end "<<real_count<<","<<real_match<<std::endl;
    }
    return true;

}

bool PsmIndexer::DoMatch(const std::string& scd_path, const std::string& knowledge_dir)
{
    std::string done_file = knowledge_dir+"/index.done";
    if(!boost::filesystem::exists(done_file))
    {
        LOG(ERROR)<<"psm index not done "<<done_file<<std::endl;
        return false;
    }
    std::vector<std::string> scd_list;
    B5MHelper::GetIUScdList(scd_path, scd_list);
    if(scd_list.empty())
    {
        LOG(INFO)<<"match scd empty"<<std::endl;
        return true;
    }
    CategoryStringMatcher cs_matcher(true);//reverse
    std::string category_file = knowledge_dir+"/category";
    if(boost::filesystem::exists(category_file))
    {
        std::ifstream cifs(category_file.c_str());
        std::string line;
        while(getline(cifs, line))
        {
            boost::algorithm::trim(line);
            LOG(INFO)<<"find category regex : "<<line<<std::endl;
            cs_matcher.AddCategoryRegex(line); }
        cifs.close();
    }

    std::string psm_path = knowledge_dir+"/psm";
    if(!boost::filesystem::exists(psm_path))
    {
        LOG(ERROR)<<"psm not ready"<<std::endl;
        return false;
    }
    PsmType psm(psm_path);
    psm.SetK(psmk_);
    psm.Open();
    LOG(INFO)<<"psm opened"<<std::endl;
    ProductTermAnalyzer analyzer(cma_path_);
    std::string match_file = knowledge_dir+"/match";
    std::ofstream match_ofs(match_file.c_str());
    for(uint32_t i=0;i<scd_list.size();i++)
    {
        std::string scd_file = scd_list[i];
        LOG(INFO)<<"Testing "<<scd_file<<std::endl;
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
            std::map<std::string, UString> doc;
            SCDDoc& scddoc = *(*doc_iter);
            SCDDoc::iterator p = scddoc.begin();
            for(; p!=scddoc.end(); ++p)
            {
                const std::string& property_name = p->first;
                doc[property_name] = p->second;
            }
            if(doc["uuid"].length()>0) continue;
            std::string key;
            std::vector<std::pair<std::string, double> > doc_vector;
            PsmAttach attach;
            if(!PsmHelper::GetPsmItem(cs_matcher, analyzer, doc, key, doc_vector, attach))
            {
                continue;
            }
            std::string match_key;
            if(psm.Search(doc_vector, attach, match_key))
            {
                boost::posix_time::ptime now = boost::posix_time::second_clock::local_time();
                std::string time_str = boost::posix_time::to_iso_string(now);
                match_ofs<<key<<","<<match_key<<","<<time_str<<std::endl;
            }
        }
    }
    match_ofs.close();
    return true;
}

