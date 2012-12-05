#include "FMIndexManager.h"
#include <document-manager/DocumentManager.h>
#include <boost/filesystem.hpp>
#include <glog/logging.h>
#include <icma/icma.h>
#include <la-manager/LAPool.h>
#include <mining-manager/util/split_ustr.h>
#include <mining-manager/group-manager/DateStrFormat.h>
#include "FilterManager.h"

using namespace cma;
namespace sf1r
{
using namespace faceted;

FMIndexManager::FMIndexManager(const std::string& homePath, boost::shared_ptr<DocumentManager>& document_manager)
    : data_root_path_(homePath)
    , document_manager_(document_manager)
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
}

bool FMIndexManager::loadAll()
{
    for(FMIndexIter fmit = all_fmi_.begin(); fmit != all_fmi_.end() ; ++fmit)
    {
        std::ifstream ifs((data_root_path_ + "/" + fmit->first + ".fm_idx").c_str());
        if(ifs)
        {
            fmit->second.fmi.reset(new FMIndexType);
            fmit->second.fmi->load(ifs);
            //if(fmit->second.type == COMMON)
            //{
            //    size_t cnt = fmit->second.fmi->docCount();
            //    if(doc_count_ == 0)
            //        doc_count_ = cnt;
            //    else
            //    {
            //        assert(cnt == doc_count_);
            //        if(cnt != doc_count_)
            //        {
            //            LOG(ERROR) << "the doccount of fmindex is not the same size on property : " << fmit->first << ", " << cnt;
            //            clearFMIData();
            //            return false;
            //        }
            //    }
            //}
        }
    }
    try
    {
        std::ifstream ifs((data_root_path_ + "/" + "AllDocArray.doc_array").c_str());
        if(ifs)
        {
            docarray_mgr_.load(ifs);
            doc_count_ = docarray_mgr_.getDocCount();
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
    docarray_mgr_.setDocCount(doc_count_);
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
    }
    LOG(INFO) << "inserted docs: " << document_manager_->getMaxDocId();
    // load distinct value for less_distinct property.
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
    for(; it_start != it_end; ++it_start)
    {
        it_start->second.fmi.reset(new FMIndexType());
        if(it_start->second.type != COMMON)
            continue;

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
    doc_array_item.doc_delim.swap(fmi->getDocDelim());
    doc_array_item.doc_array_ptr.swap(fmi->getDocArray());
    return mgr_index;
}

bool FMIndexManager::buildCollection(const FMIndexManager* old_fmi_manager)
{
    if(!initAndLoadOldDocs(old_fmi_manager))
    {
        LOG(ERROR) << "fmindex init building failed, must stop. ";
        clearFMIData();
        return false;
    }

    appendDocs(doc_count_);

    FMIndexIter it_start = all_fmi_.begin();
    FMIndexIter it_end = all_fmi_.end();
    size_t new_doc_cnt = 0;
    doc_count_ = 0;
    for(; it_start != it_end; ++it_start)
    {
        std::ofstream ofs((data_root_path_ + "/" + it_start->first + ".orig_txt").c_str());
        it_start->second.fmi->saveOriginalText(ofs);
        ofs.close();
        LOG(INFO) << "building fm-index for property : " << it_start->first << " ....";
        it_start->second.fmi->build();
        if(it_start->second.type == COMMON)
        {
            new_doc_cnt = it_start->second.fmi->docCount();
            if(doc_count_ == 0)
            {
                doc_count_ = new_doc_cnt;
            }
            else if(new_doc_cnt != doc_count_)
            {
                LOG(ERROR) << "docCount is different in common property: " << it_start->first;
                clearFMIData();
                return false;
            }
            docarray_mgr_.setDocCount(doc_count_);
            it_start->second.docarray_mgr_index = putFMIndexToDocArrayMgr(it_start->second.fmi.get());
        }
        LOG(INFO) << "building fm-index for property : " << it_start->first << " finished, docCount:" << doc_count_;
    }
    return true;
}

void FMIndexManager::getMatchedDocIdList(
    const std::string& property,
    const FilterManager* filter_manager,
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
        size_t match_filter_index = filter_manager->getPropertyFilterIndex(property);
        if(match_filter_index == (size_t)-1)
        {
            LOG(ERROR) << "the LESS_DV property : " << property << "  not found in filter.";
            return;
        }
        docarray_mgr_.getMatchedDocIdList(match_filter_index, true, match_ranges,
            max_docs, docid_list, doclen_list);
    }
    else
    {
        assert(false);
    }

}

void FMIndexManager::convertMatchRanges(
        const std::string& property,
        const FilterManager* filter_manager,
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
    if(cit->second.type != LESS_DV)
        return;
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
        converted_match_ranges.reserve(converted_match_ranges.size() + tmp_docid_list.size());
        converted_max_match_list.reserve(converted_max_match_list.size() + tmp_docid_list.size());
        for(size_t j = 0; j < tmp_docid_list.size(); ++j)
        {
            FilterManager::FilterIdRange range = filter_manager->getNumFilterIdRangeExactly(property, tmp_docid_list[j]);
            cout << "( id: " << tmp_docid_list[j] << ", converted range: " << range.start << "-" << range.end << ")" << std::flush;
            if(range.start >= range.end)
                continue;
            converted_match_ranges.push_back(std::make_pair(range.start, range.end));
            converted_max_match_list.push_back(max_match_list[i]);
        }
        cout << endl;
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
    if(cit == all_fmi_.end())
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
    if(cit == all_fmi_.end())
    {
        return 0;
    }
    return cit->second.fmi->backwardSearch(pattern.data(), pattern.length(), match_range);
}
void FMIndexManager::getTopKDocIdListByFilter(
    const std::string& property,
    const FilterManager* filter_manager,
    const std::vector<size_t> &filter_index_list,
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
        return;
    }
    if(cit->second.type == COMMON)
    {
        if(cit->second.docarray_mgr_index == (size_t)-1)
        {
            LOG(ERROR) << "the common property : " << property << "  not found in doc array.";
            return;
        }
        docarray_mgr_.getTopKDocIdListByFilter(filter_index_list, filter_ranges,
            cit->second.docarray_mgr_index, false, match_ranges_list, max_match_list,
            max_docs, res_list);
    }
    else if(cit->second.type == LESS_DV)
    {
        size_t match_filter_index = filter_manager->getPropertyFilterIndex(property);
        if(match_filter_index == (size_t)-1)
        {
            LOG(ERROR) << "the LESS_DV property : " << property << "  not found in filter.";
            return;
        }
        docarray_mgr_.getTopKDocIdListByFilter(filter_index_list, filter_ranges,
            match_filter_index, true, match_ranges_list, max_match_list, max_docs,
            res_list);
    }
    else
    {
        assert(false);
    }
}

void FMIndexManager::saveAll()
{
    FMIndexConstIter fmit = all_fmi_.begin();
    while(fmit != all_fmi_.end())
    {
        std::ofstream ofs;
        ofs.open((data_root_path_ + "/" + fmit->first + ".fm_idx").c_str());
        ofs.write((const char*)fmit->second.docarray_mgr_index, sizeof(fmit->second.docarray_mgr_index));
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
