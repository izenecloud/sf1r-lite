#include "TuanEnricher.hpp"
#include <common/type_defs.h>
#include <b5m-manager/product_matcher.h>
#include <util/profiler/ProfilerGroup.h>
#include <document-manager/DocContainer.h>

#include <icma/icma.h>
#include <la-manager/LAPool.h>

#include <glog/logging.h>
#include <set>

using namespace cma;

namespace sf1r
{
TuanEnricher::TuanEnricher(
    const std::string& resource, 
    cma::Analyzer* analyzer, 
    cma::Knowledge* knowledge)
    :resource_path_(resource)
    ,analyzer_(analyzer)
    ,knowledge_(knowledge)
    ,doc_container_(NULL)
    ,documentCache_(20000)
{
    Open_(resource_path_);
}

TuanEnricher::~TuanEnricher()
{
    if(doc_container_) delete doc_container_;
}

void TuanEnricher::Open_(const std::string& resource)
{
    LOG(INFO)<<"Loading SPU classifier resource "<<resource_path_;

    resource_path_ = resource;

    doc_container_ = new DocContainer(resource_path_+"/dm/");
    doc_container_->open();

    std::string cma_path;
    LAPool::getInstance()->get_cma_path(cma_path);
    boost::filesystem::path cma_fmindex_dic(cma_path);
    cma_fmindex_dic /= boost::filesystem::path("fmindex_dic");
    LOG(INFO) << "fm-index dictionary path : " << cma_fmindex_dic.c_str() << endl;
    knowledge_ = CMA_Factory::instance()->createKnowledge();
    knowledge_->loadModel( "utf8", cma_fmindex_dic.c_str(), false);
    analyzer_ = CMA_Factory::instance()->createAnalyzer();
    analyzer_->setOption(Analyzer::OPTION_TYPE_POS_TAGGING, 0);
    // using the maxprefix analyzer
    analyzer_->setOption(Analyzer::OPTION_ANALYSIS_TYPE, 100);
    analyzer_->setKnowledge(knowledge_);
    LOG(INFO) << "load dictionary knowledge finished." << endl;

    properties_.push_back("Title");
    for (size_t i = 0; i < properties_.size(); ++i)
    {
        const std::string& index_prop = properties_[i];
        std::pair<FMIndexIter, bool> insert_ret = all_fmi_.insert(std::make_pair(index_prop, PropertyFMIndex()));
        if (!insert_ret.second)
        {
            LOG(ERROR) << "duplicate fm index property : " << index_prop;
            continue;
        }
    }

    data_root_path_ = resource_path_ + "/suffix_match";
    LOG(INFO) << "loading spu fmindex from: " << data_root_path_;

    for (FMIndexIter fmit = all_fmi_.begin(); fmit != all_fmi_.end() ; ++fmit)
    {
        std::ifstream ifs((data_root_path_ + "/" + fmit->first + ".fm_idx").c_str());
        if (ifs)
        {
            LOG(INFO) << "loading fmindex for property : " << fmit->first;
            fmit->second.fmi.reset(new FMIndexType);
            ifs.read((char*)&fmit->second.docarray_mgr_index, sizeof(fmit->second.docarray_mgr_index));
            fmit->second.fmi->load(ifs);
        }
    }

    try
    {
        std::ifstream ifs((data_root_path_ + "/" + "AllDocArray.doc_array").c_str());
        if (ifs)
        {
            docarray_mgr_.load(ifs);
            doc_count_ = docarray_mgr_.getDocCount();
            LOG(INFO) << data_root_path_ << " loading all doc array : " << doc_count_;
        }
    }
    catch (...)
    {
        LOG(ERROR) << "load the doc array of fmindex error.";
    }
}

void TuanEnricher::GetEnrichedQuery(
    const std::string& query,
    std::string& enriched_result)
{
    std::vector<std::pair<double, uint32_t> > res_list;
    std::map<uint32_t, double> res_list_map;
    std::vector<std::pair<size_t, size_t> > match_ranges_list;
    std::vector<std::pair<double, uint32_t> > single_res_list;
    std::vector<double> max_match_list;

    size_t max_docs = 50;
    UString pattern(query, UString::UTF_8);
    Algorithm<UString>::to_lower(pattern);
    std::string pattern_str;
    pattern.convertString(pattern_str, UString::UTF_8);
    LOG(INFO) << "original query string: " << pattern_str;
    Sentence pattern_sentence(pattern_str.c_str());
    analyzer_->runWithSentence(pattern_sentence);
    std::vector<UString> all_sub_strpatterns;
    for(int i = 0; i < pattern_sentence.getCount(0); ++i)
        all_sub_strpatterns.push_back(UString(pattern_sentence.getLexicon(0,i), UString::UTF_8));

    std::vector<size_t> prop_id_list;
    std::vector<RangeListT> filter_range_list;
    size_t total_match = 0;

    for (size_t prop_i = 0; prop_i < properties_.size(); ++prop_i)
    {
        const std::string& search_property = properties_[prop_i];
        match_ranges_list.reserve(all_sub_strpatterns.size());
        max_match_list.reserve(all_sub_strpatterns.size());
        LOG(INFO) << "query tokenize match ranges in property : " << search_property;
        FMIndexConstIter cit = all_fmi_.find(search_property);
        for (size_t i = 0; i < all_sub_strpatterns.size(); ++i)
        {
            if (all_sub_strpatterns[i].empty())
                continue;
            const UString& pattern = all_sub_strpatterns[i];
            std::pair<size_t, size_t> sub_match_range;
            size_t matched = cit->second.fmi->backwardSearch(pattern.data(), pattern.length(), sub_match_range);
            LOG(INFO) << "match length: " << matched << ", range:" << sub_match_range.first << "," << sub_match_range.second << endl;
            if (matched == all_sub_strpatterns[i].length())
            {
                match_ranges_list.push_back(sub_match_range);
                max_match_list.push_back((double)pattern.length());
            }
        }
        docarray_mgr_.getTopKDocIdListByFilter(
            prop_id_list,
            filter_range_list,
            cit->second.docarray_mgr_index,
            false,
            match_ranges_list,
            max_match_list,
            max_docs,
            single_res_list);

        //LOG(INFO) << "topk finished in property : " << search_property;
        for (size_t i = 0; i < single_res_list.size(); ++i)
        {
            res_list_map[single_res_list[i].second] += single_res_list[i].first;
        }
        single_res_list.clear();

        for (size_t i = 0; i < match_ranges_list.size(); ++i)
        {
            total_match += match_ranges_list[i].second - match_ranges_list[i].first;
        }
        match_ranges_list.clear();
        max_match_list.clear();
        //LOG(INFO) << "new added docid number: " << res_list_map.size() - oldsize;
    }

    res_list.reserve(res_list_map.size());
    for (std::map<uint32_t, double>::const_iterator cit = res_list_map.begin();
            cit != res_list_map.end(); ++cit)
    {
        res_list.push_back(std::make_pair(cit->second, cit->first));
    }
    std::sort(res_list.begin(), res_list.end(), std::greater<std::pair<double, uint32_t> >());
    if (res_list.size() > max_docs)
        res_list.erase(res_list.begin() + max_docs, res_list.end());

    size_t topK = res_list.size() < 4 ? res_list.size() : 4;
    for(size_t i = 0; i < topK; ++i)
    {
        uint32_t docId = res_list[i].second;
      //LOG(INFO) <<"doc "<<docId;
        Document doc;
        if (!documentCache_.getValue(docId, doc))
        {
            if(!doc_container_->get(docId, doc))
                continue;
            else
                documentCache_.insertValue(docId, doc);
        }
        
        UString enriched = doc.property("Title").get<izenelib::util::UString>();
        enriched += UString(" ");
        enriched += doc.property("Category").get<izenelib::util::UString>();
        enriched += UString(" ");		
        enriched += doc.property("SubCategory").get<izenelib::util::UString>();

        std::string enriched_str;
        enriched.convertString(enriched_str, UString::UTF_8);
        
        //LOG(INFO) <<"enriched_str "<<enriched_str<<docId;
        enriched_result += std::string(" ");
        enriched_result += enriched_str;
    }
}

}

