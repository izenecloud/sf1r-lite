#include "FMIndexManager.h"
#include <document-manager/DocumentManager.h>
#include <boost/filesystem.hpp>
#include <glog/logging.h>
#include <icma/icma.h>
#include <la-manager/LAPool.h>
#include <mining-manager/util/split_ustr.h>
#include <mining-manager/group-manager/DateStrFormat.h>
#include "FilterManager.h"

namespace
{
struct RangeSorter
{
    bool operator()(const sf1r::FMIndexManager::RangeT& t1, const sf1r::FMIndexManager::RangeT& t2)
    {
        return t1.first < t2.first ||
            (t1.first == t2.first && t1.second < t2.second);
    }
} RangeSorter_;
}

using namespace cma;
namespace sf1r
{
using namespace faceted;

FMIndexManager::FMIndexManager(const std::string& homePath,
    boost::shared_ptr<DocumentManager>& document_manager,
    boost::shared_ptr<FilterManager>& filter_manager)
    : data_root_path_(homePath)
    , document_manager_(document_manager)
    , filter_manager_(filter_manager)
    , doc_count_(0)
{
}

FMIndexManager::~FMIndexManager()
{
}

bool FMIndexManager::isStartFromLocalFM() const
{
    return doc_count_ > 0;
}

void FMIndexManager::addProperties(const std::vector<std::string>& properties, PropertyFMType type)
{
    for(size_t i = 0; i < properties.size(); ++i)
    {
        const std::string& index_prop = properties[i];
        std::pair<FMIndexIter, bool> insert_ret = all_fmi_.insert(std::make_pair(index_prop, PropertyFMIndex()));
        if(!insert_ret.second)
        {
            LOG(ERROR) << "duplicate fm index property : " << index_prop;
            continue;
        }
        insert_ret.first->second.type = type;
    }
}

void FMIndexManager::getProperties(std::vector<std::string>& properties, PropertyFMType type) const
{
    FMIndexConstIter cit = all_fmi_.begin();
    while(cit != all_fmi_.end())
    {
        if(cit->second.type == type)
            properties.push_back(cit->first);
        ++cit;
    }
}

void FMIndexManager::clearFMIData()
{
    for(FMIndexIter fmit = all_fmi_.begin(); fmit != all_fmi_.end() ; ++fmit)
    {
        fmit->second.fmi.reset();
        fmit->second.docarray_mgr_index = (size_t)(-1);
    }
    docarray_mgr_.clear();
    doc_count_ = 0;
    LOG(INFO) << "fmi data cleared!";
}

bool FMIndexManager::loadAll()
{
    for(FMIndexIter fmit = all_fmi_.begin(); fmit != all_fmi_.end() ; ++fmit)
    {
        std::ifstream ifs((data_root_path_ + "/" + fmit->first + ".fm_idx").c_str());
        if(ifs)
        {
            LOG(INFO) << "loading fmindex for property : " << fmit->first;
            fmit->second.fmi.reset(new FMIndexType);
            ifs.read((char*)&fmit->second.docarray_mgr_index, sizeof(fmit->second.docarray_mgr_index));
            fmit->second.fmi->load(ifs);
            if(fmit->second.docarray_mgr_index == (size_t)-1)
            {
                // the LESS_DV property hold its own doc_array data.
                if(fmit->second.type != LESS_DV)
                {
                    LOG(ERROR) << "the doc array should be stored to doc array manager for Not LESS_DV property: " << fmit->first;
                    clearFMIData();
                    return false;
                }
                if(fmit->second.fmi->docCount() == 0)
                {
                    LOG(ERROR) << "the LESS_DV property's doc array count is zero!!";
                }
            }
            else
            {
                if(fmit->second.type == LESS_DV)
                {
                    LOG(ERROR) << "Doc array should not be stored to doc array manager for LESS_DV property: " << fmit->first;
                    clearFMIData();
                    return false;
                }
                if(fmit->second.fmi->docCount() != 0 )
                {
                    LOG(ERROR) << "the COMMON property's doc array count must be zero!!";
                    clearFMIData();
                    return false;
                }
            }
        }
    }
    try
    {
        std::ifstream ifs((data_root_path_ + "/" + "AllDocArray.doc_array").c_str());
        if(ifs)
        {
            docarray_mgr_.load(ifs);
            doc_count_ = docarray_mgr_.getDocCount();
            LOG(INFO) << data_root_path_ << " loading all doc array : " << doc_count_;
        }
    }
    catch(...)
    {
        LOG(ERROR) << "load the doc array of fmindex error.";
        clearFMIData();
        return false;
    }
    return true;
}

void FMIndexManager::buildExternalFilter()
{
    if(doc_count_ == 0)
    {
        LOG(ERROR) << "the doc count should not be 0 before building the external filter.";
        return;
    }
    docarray_mgr_.buildFilter();
}

void FMIndexManager::setFilterList(std::vector<std::vector<FMDocArrayMgrType::FilterItemT> > &filter_list)
{
    docarray_mgr_.setFilterList(filter_list);
}

bool FMIndexManager::getFilterRange(size_t filter_id, const RangeT &filter_id_range, RangeT &match_range) const
{
    return docarray_mgr_.getFilterRange(filter_id, filter_id_range, match_range);
}

void FMIndexManager::appendDocs(size_t last_docid)
{
    // add new document data to fm index.
    for (size_t i = last_docid + 1; i <= document_manager_->getMaxDocId(); ++i)
    {
        if (i % 100000 == 0)
        {
            LOG(INFO) << "inserted docs: " << i;
        }
        bool failed = false;
        Document doc;
        if (!document_manager_->getDocument(i, doc))
        {
            failed = true;
        }
        appendDocsAfter(failed, doc);
        
    }
    LOG(INFO) << "inserted docs: " << document_manager_->getMaxDocId();
}

void FMIndexManager::appendDocsAfter(bool failed, const Document& doc)
{
    FMIndexIter it_start = all_fmi_.begin();
    FMIndexIter it_end = all_fmi_.end();
    while(it_start != it_end)
    {
        if(it_start->second.type == COMMON)
        {
            if(failed)
            {
                it_start->second.fmi->addDoc(NULL, 0);
            }
            else
            {
                const std::string& prop_name = it_start->first;
                Document::property_const_iterator it = doc.findProperty(prop_name);
                if (it == doc.propertyEnd())
                {
                    it_start->second.fmi->addDoc(NULL, 0);
                }
                else
                {
                    izenelib::util::UString text = it->second.get<UString>();
                    Algorithm<UString>::to_lower(text);
                    text = Algorithm<UString>::trim(text);
                    it_start->second.fmi->addDoc(text.data(), text.length());
                }
            }
        }
        ++it_start;
    }
    //LOG(INFO) << "inserted docs: " << document_manager_->getMaxDocId();
}

void FMIndexManager::reconstructText(const std::string& prop_name,
    const std::vector<uint32_t>& del_docid_list,
    std::vector<uint16_t>& orig_text) const
{
    if(doc_count_ == 0)
        return;
    FMIndexConstIter cit = all_fmi_.find(prop_name);
    if(cit == all_fmi_.end())
        return;
    if(cit->second.type != COMMON)
        return;
    docarray_mgr_.reconstructText(cit->second.docarray_mgr_index, del_docid_list, orig_text);
}

bool FMIndexManager::initAndLoadOldDocs(const FMIndexManager* old_fmi_manager)
{
    std::vector<uint32_t> del_docid_list;
    document_manager_->getDeletedDocIdList(del_docid_list);
    FMIndexIter it_start = all_fmi_.begin();
    FMIndexIter it_end = all_fmi_.end();

    int has_orig_prop = 0;
    if(old_fmi_manager)
    {
        doc_count_ = old_fmi_manager->docCount();
    }
    for(; it_start != it_end; ++it_start)
    {
        if(it_start->second.type != COMMON)
            continue;

        it_start->second.fmi.reset(new FMIndexType());
        if(doc_count_ == 0 || old_fmi_manager == NULL)
            continue;

        const std::string& prop_name = it_start->first;
        FMIndexType* new_fmi = it_start->second.fmi.get();
        LOG(INFO) << "start loading orig_text in fm-index for property : " << prop_name;
        std::ifstream ifs((data_root_path_ + "/" + prop_name + ".orig_txt").c_str());
        if (ifs)
        {
            new_fmi->loadOriginalText(ifs);
            std::vector<uint16_t> orig_text;
            new_fmi->swapOrigText(orig_text);
            old_fmi_manager->reconstructText(prop_name, del_docid_list, orig_text);
            new_fmi->swapOrigText(orig_text);
            has_orig_prop++;
        }
        else
        {
            LOG(ERROR) << "one of fmindex's orig_txt is empty, must rebuilding from 0";
            doc_count_ = 0;
            // all properties should have orig_txt or none property have orig_txt.
            // if not, it should be some wrong and the build need stop.
            if(has_orig_prop > 0)
                return false;
        }
    }
    return true;
}

size_t FMIndexManager::putFMIndexToDocArrayMgr(FMIndexType* fmi)
{
    // the doc array added to manager should all be the same doc size.
    size_t mgr_index = docarray_mgr_.mainDocArrayNum();
    FMDocArrayMgrType::DocArrayItemT& doc_array_item = docarray_mgr_.addMainDocArrayItem();
    // take the owner of data to avoid duplication.
    LOG(INFO) << "add the doc array to manager, docCount: " << fmi->docCount(); 
    doc_array_item.doc_delim.swap(fmi->getDocDelim());
    doc_array_item.doc_array_ptr.swap(fmi->getDocArray());
    return mgr_index;
}

void FMIndexManager::buildLessDVProperties()
{
    // Indexing: the LESS_DV property docid list will be stored in the doc array manager.
    // The docid list wavelet tree can be generated from the inverted docid list in the 
    // FilterManager. In FMIndex, only store the distinct property value id for 
    // each distinct property value string.
    // Searching: from distinct value string we can get the distinct property value id  
    // by searching the fmindex, and using the distinct property value id we can
    // get the filter range in the docarray of docid. Using the range we can 
    // get topk docid list by doing union/intersect with other ranges.
    if(!filter_manager_)
    {
        LOG(INFO) << "filter manager is empty, LESS_DV property will not build.";
        return;
    }
    FMIndexIter it_start = all_fmi_.begin();
    FMIndexIter it_end = all_fmi_.end();
    for( ; it_start != it_end; ++it_start)
    {
        if(it_start->second.type != LESS_DV)
            continue;
        it_start->second.fmi.reset(new FMIndexType());
        size_t prop_id = filter_manager_->getPropertyId(it_start->first);
        if(prop_id == (size_t)-1)
        {
            LOG(ERROR) << "LESS_DV property not found in filter manager : " << it_start->first;
            continue;
        }
        // read all property distinct string from filter manager.
        size_t max_filterstr_id = filter_manager_->getMaxPropFilterStrId(prop_id);
        for(size_t i = 1; i <= max_filterstr_id; ++i)
        {
            izenelib::util::UString text = filter_manager_->getPropFilterString(prop_id, i);
            Algorithm<UString>::to_lower(text);
            text = Algorithm<UString>::trim(text);
            it_start->second.fmi->addDoc(text.data(), text.length());
        }
        LOG(INFO) << "LESS_DV for property count is : " << max_filterstr_id ;
        LOG(INFO) << "building fm-index for property : " << it_start->first << " ....";
        it_start->second.fmi->build();
        LOG(INFO) << "building fm-index for property : " << it_start->first << " finished, docCount:" << doc_count_;
    }
}

void FMIndexManager::swapCommonPropertiesData(FMIndexManager* old_fmi_manager)
{
    std::swap(doc_count_, old_fmi_manager->doc_count_);
    LOG(INFO) << " swap common fmindex data, doc count: " << doc_count_;
    docarray_mgr_.swapMainDocArray(old_fmi_manager->docarray_mgr_);

    FMIndexIter it_start = all_fmi_.begin();
    FMIndexIter it_end = all_fmi_.end();
    FMIndexIter old_it_start = old_fmi_manager->all_fmi_.begin();
    FMIndexIter old_it_end = old_fmi_manager->all_fmi_.end();
    for(; it_start != it_end && old_it_start != old_it_end; ++it_start, ++old_it_start)
    {
        if(it_start->second.type != COMMON)
            continue;
        it_start->second.fmi.swap(old_it_start->second.fmi);
        std::swap(it_start->second.docarray_mgr_index, old_it_start->second.docarray_mgr_index);
    }
}

void FMIndexManager::useOldDocCount(const FMIndexManager* old_fmi_manager)
{
    doc_count_ = old_fmi_manager->doc_count_;
    docarray_mgr_.setDocCount(doc_count_);
}

bool FMIndexManager::buildCommonProperties(const FMIndexManager* old_fmi_manager)
{
    if(!initAndLoadOldDocs(old_fmi_manager))
    {
        LOG(ERROR) << "fmindex init building failed, must stop. ";
        clearFMIData();
        return false;
    }

    appendDocs(doc_count_);

    return buildCollectionAfter();
}

bool FMIndexManager::buildCollectionAfter()
{
    FMIndexIter it_start = all_fmi_.begin();
    FMIndexIter it_end = all_fmi_.end();
    size_t new_doc_cnt = 0;
    doc_count_ = 0;
    docarray_mgr_.clearMainDocArray();
    for(; it_start != it_end; ++it_start)
    {
        if(it_start->second.type != COMMON)
            continue;
        std::ofstream ofs((data_root_path_ + "/" + it_start->first + ".orig_txt").c_str());
        it_start->second.fmi->saveOriginalText(ofs);
        ofs.close();
        LOG(INFO) << "building fm-index for property : " << it_start->first << " ....";
        it_start->second.fmi->build();
        new_doc_cnt = it_start->second.fmi->docCount();
        if(doc_count_ == 0)
        {
            doc_count_ = new_doc_cnt;
            docarray_mgr_.setDocCount(doc_count_);
        }
        else if(new_doc_cnt != doc_count_)
        {
            LOG(ERROR) << "docCount is different in common property: " << it_start->first;
            clearFMIData();
            return false;
        }
        it_start->second.docarray_mgr_index = putFMIndexToDocArrayMgr(it_start->second.fmi.get());
        LOG(INFO) << "building fm-index for property : " << it_start->first << " finished, docCount:" << doc_count_;
    }
    return true;
}

void FMIndexManager::getMatchedDocIdList(
    const std::string& property,
    const RangeListT& match_ranges, size_t max_docs,
    std::vector<uint32_t>& docid_list, std::vector<size_t>& doclen_list) const
{
    if(doc_count_ == 0)
        return;
    // if the property is LESS_DV type, find in the filter docid list
    // otherwise find in the fm index.
    FMIndexConstIter cit = all_fmi_.find(property);
    if(cit == all_fmi_.end())
    {
        return;
    }
    if(cit->second.type == COMMON)
    {
        if(cit->second.docarray_mgr_index == (size_t)-1)
        {
            LOG(ERROR) << "the common property : " << property << "  not found in doc array.";
            return;
        }
        docarray_mgr_.getMatchedDocIdList(cit->second.docarray_mgr_index, false, match_ranges,
            max_docs, docid_list, doclen_list);
    }
    else if(cit->second.type == LESS_DV)
    {
        if(!filter_manager_)
            return;
        size_t match_filter_index = filter_manager_->getPropertyId(property);
        if(match_filter_index == (size_t)-1)
        {
            LOG(ERROR) << "the LESS_DV property : " << property << "  not found in filter.";
            return;
        }
        docarray_mgr_.getMatchedDocIdList(match_filter_index, true, match_ranges,
            max_docs, docid_list, doclen_list);
        // Since doclen is not available in doc array manager for LESS_DV property,
        // we need recompute doclen from common property.
        getDocLenList(docid_list, doclen_list);
    }
    else
    {
        assert(false);
    }

}

void FMIndexManager::convertMatchRanges(
        const std::string& property,
        size_t max_docs,
        RangeListT& match_ranges,
        std::vector<double>& max_match_list) const
{
    if(doc_count_ == 0)
        return;
    FMIndexConstIter cit = all_fmi_.find(property);
    if(cit == all_fmi_.end())
    {
        return;
    }
    if(cit->second.type != LESS_DV || (filter_manager_ == NULL))
        return;
    if(!cit->second.fmi)
    {
        match_ranges.clear();
        max_match_list.clear();
        return;
    }
    std::vector<uint32_t> tmp_docid_list;
    std::vector<size_t> tmp_doclen_list;
    RangeListT converted_match_ranges;
    std::vector<double> converted_max_match_list;
    LOG(INFO) << "convert range for property : " << property;
    for(size_t i = 0; i < match_ranges.size(); ++i)
    {
        LOG(INFO) << "orig range : " << match_ranges[i].first << ", " << match_ranges[i].second;
        cit->second.fmi->getMatchedDocIdList(match_ranges[i], max_docs, tmp_docid_list, tmp_doclen_list);
        // for LESS_DV, the docid is the distinct value id, we need get all the docid belong to the distinct
        // value from the docarray_mgr_.
        size_t oldsize = converted_match_ranges.size();
        converted_match_ranges.reserve(oldsize + tmp_docid_list.size());
        converted_max_match_list.reserve(oldsize + tmp_docid_list.size());
        std::sort(tmp_docid_list.begin(), tmp_docid_list.end(), std::less<uint32_t>());
        for(size_t j = 0; j < tmp_docid_list.size(); ++j)
        {
            size_t prop_id = filter_manager_->getPropertyId(property);
            if(prop_id == (size_t)-1)
            {
                LOG(ERROR) << "property id not found in FilterManager, " << property;
                continue;
            }
            UString match_filter_string = filter_manager_->getPropFilterString(prop_id, tmp_docid_list[j]);
            if(match_filter_string.empty())
                continue;
            FilterManager::FilterIdRange range = filter_manager_->getStrFilterIdRange(
                prop_id, match_filter_string);
            if(range.start >= range.end)
                continue;
            RangeT doc_array_filterrange;
            getFilterRange(prop_id, std::make_pair(range.start, range.end), doc_array_filterrange);
            LOG(INFO) << "( id: " << tmp_docid_list[j] << ", converted match range: " << doc_array_filterrange.first <<
                "-" << doc_array_filterrange.second << ")";
            if(converted_match_ranges.size() > oldsize)
            {
                RangeT& prev_range = converted_match_ranges.back();
                if(doc_array_filterrange.first < prev_range.first)
                {
                    LOG(ERROR) << "range should be after.!! " << doc_array_filterrange.first <<
                        "-" << doc_array_filterrange.second << ", prev_range: " <<
                        prev_range.first << "-" << prev_range.second;
                }
                // merge the overlap range.
                if(doc_array_filterrange.first <= prev_range.second)
                {
                    prev_range.second = max(doc_array_filterrange.second, prev_range.second);
                    continue;
                }
            }
            converted_match_ranges.push_back(doc_array_filterrange);
            converted_max_match_list.push_back(max_match_list[i]);
        }
        std::vector<uint32_t>().swap(tmp_docid_list);
        std::vector<size_t>().swap(tmp_doclen_list);
    }
    converted_match_ranges.swap(match_ranges);
    converted_max_match_list.swap(max_match_list);
}

size_t FMIndexManager::longestSuffixMatch(
        const std::string& property,
        const izenelib::util::UString& pattern,
        RangeListT& match_ranges) const
{
    if(doc_count_ == 0)
        return 0;
    FMIndexConstIter cit = all_fmi_.find(property);
    if(cit == all_fmi_.end() || !cit->second.fmi)
    {
        return 0;
    }
    return cit->second.fmi->longestSuffixMatch(pattern.data(), pattern.length(), match_ranges);
}

size_t FMIndexManager::backwardSearch(const std::string& prop, const izenelib::util::UString& pattern, RangeT& match_range) const
{
    if(doc_count_ == 0)
        return 0;

    FMIndexConstIter cit = all_fmi_.find(prop);
    if(cit == all_fmi_.end() || !cit->second.fmi)
    {
        return 0;
    }
    return cit->second.fmi->backwardSearch(pattern.data(), pattern.length(), match_range);
}
void FMIndexManager::getTopKDocIdListByFilter(
    const std::string& property,
    const std::vector<size_t> &prop_id_list,
    const std::vector<RangeListT> &filter_ranges,
    const RangeListT &match_ranges_list,
    const std::vector<double> &max_match_list,
    size_t max_docs,
    std::vector<std::pair<double, uint32_t> > &res_list) const
{
    if(doc_count_ == 0)
        return;

    FMIndexConstIter cit = all_fmi_.find(property);
    if(cit == all_fmi_.end())
    {
        LOG(INFO) << "get topk failed for not exist property : " << property;
        return;
    }
    if(cit->second.type == COMMON)
    {
        LOG(INFO) << "get topk in common property : " << property;
        if(cit->second.docarray_mgr_index == (size_t)-1)
        {
            LOG(ERROR) << "the common property : " << property << "  not found in doc array.";
            return;
        }
        docarray_mgr_.getTopKDocIdListByFilter(prop_id_list, filter_ranges,
            cit->second.docarray_mgr_index, false, match_ranges_list, max_match_list,
            max_docs, res_list);
    }
    else if(cit->second.type == LESS_DV)
    {
        if(filter_manager_ == NULL)
            return;
        LOG(INFO) << "get topk in LESS_DV property : " << property;
        size_t match_filter_index = filter_manager_->getPropertyId(property);
        if(match_filter_index == (size_t)-1)
        {
            LOG(ERROR) << "the LESS_DV property : " << property << "  not found in filter.";
            return;
        }
        docarray_mgr_.getTopKDocIdListByFilter(prop_id_list, filter_ranges,
            match_filter_index, true, match_ranges_list, max_match_list, max_docs,
            res_list);
    }
    else
    {
        assert(false);
    }
}

void FMIndexManager::getDocLenList(const std::vector<uint32_t>& docid_list, std::vector<size_t>& doclen_list) const
{
    // the doclen is total length for common property only.
    docarray_mgr_.getDocLenList(docid_list, doclen_list);
}

void FMIndexManager::getLessDVStrLenList(const std::string& property, const std::vector<uint32_t>& dvid_list, std::vector<size_t>& dvlen_list) const
{
    dvlen_list.resize(dvid_list.size(), 0);
    if(doc_count_ == 0)
        return;
    FMIndexConstIter cit = all_fmi_.find(property);
    if(cit == all_fmi_.end() || !cit->second.fmi)
    {
        LOG(INFO) << "get LESS_DV string length failed for not exist property : " << property;
        return;
    }
    if(cit->second.type != LESS_DV)
    {
        LOG(INFO) << "get LESS_DV string length failed for not LESS_DV property : " << property;
        return;
    }

    // for LESS_DV properties, we can use the distinct value id to get the length of the distinct value string.
    cit->second.fmi->getDocLenList(dvid_list, dvlen_list);
}

void FMIndexManager::saveAll()
{
    FMIndexConstIter fmit = all_fmi_.begin();
    while(fmit != all_fmi_.end())
    {
        std::ofstream ofs;
        ofs.open((data_root_path_ + "/" + fmit->first + ".fm_idx").c_str());
        ofs.write((const char*)&fmit->second.docarray_mgr_index, sizeof(fmit->second.docarray_mgr_index));
        if(fmit->second.fmi)
            fmit->second.fmi->save(ofs);
        ++fmit;
    }
    try
    {
        std::ofstream ofs;
        ofs.open((data_root_path_ + "/" + "AllDocArray.doc_array").c_str());
        docarray_mgr_.save(ofs);
    }
    catch(...)
    {
        LOG(ERROR) << "saving doc array of fmindex error. must clean crashed data.";
        clearFMIData();
        boost::filesystem::remove_all(data_root_path_);
    }
}

}
