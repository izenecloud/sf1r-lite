#include "SuffixMatchManager.hpp"
#include <document-manager/DocumentManager.h>
#include <boost/filesystem.hpp>
#include <glog/logging.h>
#include <icma/icma.h>
#include <la-manager/LAPool.h>
#include <mining-manager/util/split_ustr.h>
#include <mining-manager/group-manager/DateStrFormat.h>
#include "FilterManager.h"

using namespace cma;

namespace
{
const char* NUMERIC_RANGE_DELIMITER = "-";

typedef std::pair<int64_t, int64_t> NumericRange;

using namespace sf1r::faceted;

/**
 * check parameter of group label
 * @param[in] labelParam parameter to check
 * @param[out] isRange true for group on range, false for group on single numeric value
 * @return true for success, false for failure
*/
bool checkLabelParam(const GroupParam::GroupLabelParam& labelParam, bool& isRange)
{
    const GroupParam::GroupPathVec& labelPaths = labelParam.second;

    if (labelPaths.empty())
        return false;

    const GroupParam::GroupPath& path = labelPaths.front();
    if (path.empty())
        return false;

    const std::string& propValue = path.front();
    std::size_t delimitPos = propValue.find(NUMERIC_RANGE_DELIMITER);
    isRange = (delimitPos != std::string::npos);

    return true;
}

bool convertNumericLabel(const std::string& src, float& target)
{
    std::size_t delimitPos = src.find(NUMERIC_RANGE_DELIMITER);
    if (delimitPos != std::string::npos)
    {
        LOG(ERROR) << "group label parameter: " << src
                   << ", it should be specified as single numeric value";
        return false;
    }

    try
    {
        target = boost::lexical_cast<float>(src);
    }
    catch(const boost::bad_lexical_cast& e)
    {
        LOG(ERROR) << "failed in casting label from " << src
                   << " to numeric value, exception: " << e.what();
        return false;
    }

    return true;
}

bool convertRangeLabel(const std::string& src, NumericRange& target)
{
    std::size_t delimitPos = src.find(NUMERIC_RANGE_DELIMITER);
    if (delimitPos == std::string::npos)
    {
        LOG(ERROR) << "group label parameter: " << src
                   << ", it should be specified as numeric range value";
        return false;
    }

    int64_t lowerBound = std::numeric_limits<int64_t>::min();
    int64_t upperBound = std::numeric_limits<int64_t>::max();

    try
    {
        if (delimitPos)
        {
            std::string sub = src.substr(0, delimitPos);
            lowerBound = boost::lexical_cast<int64_t>(sub);
        }

        if (delimitPos+1 != src.size())
        {
            std::string sub = src.substr(delimitPos+1);
            upperBound = boost::lexical_cast<int64_t>(sub);
        }
    }
    catch(const boost::bad_lexical_cast& e)
    {
        LOG(ERROR) << "failed in casting label from " << src
                    << " to numeric value, exception: " << e.what();
        return false;
    }

    target.first = lowerBound;
    target.second = upperBound;

    return true;
}

}


namespace sf1r
{
using namespace faceted;

SuffixMatchManager::SuffixMatchManager(
        const std::string& homePath,
        const std::string& property,
        const std::string& dicpath,
        boost::shared_ptr<DocumentManager>& document_manager,
        faceted::GroupManager* groupmanager,
        faceted::AttrManager* attrmanager,
        NumericPropertyTableBuilder* numeric_tablebuilder)
    : data_root_path_(homePath)
    , fm_index_path_(homePath + "/" + property + ".fm_idx")
    , orig_text_path_(homePath + "/" + property + ".orig_txt")
    , property_(property)
    , tokenize_dicpath_(dicpath)
    , document_manager_(document_manager)
    , analyzer_(NULL)
    , knowledge_(NULL)
{
    if (!boost::filesystem::exists(homePath))
    {
        boost::filesystem::create_directories(homePath);
    }
    else
    {
        std::ifstream ifs(fm_index_path_.c_str());
        LOG(ERROR) << "fm-index loading.";
        if (ifs)
        {
            fmi_.reset(new FMIndexType());
            fmi_->load(ifs);
        }
        LOG(ERROR) << "fm-index loaded.";
    }
    buildTokenizeDic();

    filter_manager_.reset(new FilterManager(groupmanager, data_root_path_,
            attrmanager, numeric_tablebuilder));
}

SuffixMatchManager::~SuffixMatchManager()
{
    if (analyzer_) delete analyzer_;
    if (knowledge_) delete knowledge_;
}

void SuffixMatchManager::setGroupFilterProperties(std::vector<std::string>& property_list)
{
    group_property_list_.swap(property_list);
    std::sort(group_property_list_.begin(), group_property_list_.end());
    if (filter_manager_)
    {
        WriteLock lock(mutex_);
        filter_manager_->loadFilterId(group_property_list_);
    }
}

void SuffixMatchManager::setAttrFilterProperties(std::vector<std::string>& property_list)
{
    attr_property_list_.swap(property_list);
    std::sort(attr_property_list_.begin(), attr_property_list_.end());
    if (filter_manager_)
    {
        WriteLock lock(mutex_);
        filter_manager_->loadFilterId(attr_property_list_);
    }
}

void SuffixMatchManager::setDateFilterProperties(std::vector<std::string>& property_list)
{
    date_property_list_.swap(property_list);
    std::sort(date_property_list_.begin(), date_property_list_.end());
    if (filter_manager_)
    {
        WriteLock lock(mutex_);
        filter_manager_->loadFilterId(date_property_list_);
    }
}

void SuffixMatchManager::setNumericFilterProperties(std::vector<std::string>& property_list, std::vector<int32_t>& amp_list)
{
    assert(property_list.size() == amp_list.size());
    std::map<std::string, int32_t> num_amp_map;
    for (size_t i = 0; i < property_list.size(); ++i)
    {
        num_amp_map[property_list[i]] = amp_list[i];
    }

    if (filter_manager_)
    {
        WriteLock lock(mutex_);
        filter_manager_->loadFilterId(property_list);
        filter_manager_->setNumericAmp(num_amp_map);
    }
    numeric_property_list_.insert(property_list.begin(), property_list.end());
}

bool SuffixMatchManager::isStartFromLocalFM() const
{
    return fmi_ && fmi_->docCount() > 0;
}

void SuffixMatchManager::buildCollection()
{
    std::vector<FilterManager::StrFilterItemMapT> group_filter_map;
    std::vector<FilterManager::StrFilterItemMapT> attr_filter_map;
    std::vector<FilterManager::NumFilterItemMapT> num_filter_map;
    std::vector<FilterManager::NumFilterItemMapT> date_filter_map;

    FMIndexType* new_fmi = new FMIndexType();
    FilterManager* new_filter_manager = NULL;

    // do filter building only when filter is enable.
    if (filter_manager_)
    {
        new_filter_manager = new FilterManager(filter_manager_->getGroupManager(), data_root_path_,
                                               filter_manager_->getAttrManager(), filter_manager_->getNumericTableBuilder());
        new_filter_manager->setNumericAmp(filter_manager_->getNumericAmp());
    }

    size_t last_docid = fmi_ ? fmi_->docCount() : 0;
    uint32_t max_group_docid = 0;
    uint32_t max_attr_docid = 0;
    if (last_docid)
    {
        LOG(INFO) << "start rebuilding in fm-index";
        if (new_filter_manager)
        {
            max_group_docid = new_filter_manager->loadStrFilterInvertedData(group_property_list_, group_filter_map);
            max_attr_docid = new_filter_manager->loadStrFilterInvertedData(attr_property_list_, attr_filter_map);
        }
        std::ifstream ifs(orig_text_path_.c_str());
        if (ifs)
        {
            new_fmi->loadOriginalText(ifs);
            std::vector<uint16_t> orig_text;
            new_fmi->swapOrigText(orig_text);
            std::vector<uint32_t> del_docid_list;
            document_manager_->getDeletedDocIdList(del_docid_list);
            fmi_->reconstructText(del_docid_list, orig_text);
            new_fmi->swapOrigText(orig_text);
        }
        else
        {
            last_docid = 0;
        }
    }

    LOG(INFO) << "building filter data in fm-index, start from: " << max_group_docid;
    std::vector<std::string> numeric_property_list(numeric_property_list_.begin(), numeric_property_list_.end());
    if (new_filter_manager)
    {
        new_filter_manager->buildGroupFilterData(max_group_docid, document_manager_->getMaxDocId(),
                                                 group_property_list_, group_filter_map);
        new_filter_manager->saveStrFilterInvertedData(group_property_list_, group_filter_map);

        new_filter_manager->buildAttrFilterData(max_attr_docid, document_manager_->getMaxDocId(),
                                                attr_property_list_, attr_filter_map);
        new_filter_manager->saveStrFilterInvertedData(attr_property_list_, attr_filter_map);

        new_filter_manager->buildNumericFilterData(0, document_manager_->getMaxDocId(),
                                                   numeric_property_list, num_filter_map);
        new_filter_manager->buildDateFilterData(0, document_manager_->getMaxDocId(),
                                                date_property_list_, date_filter_map);

        new_fmi->setFilterList(new_filter_manager->getFilterList());

        std::vector<FilterManager::StrFilterItemMapT>().swap(group_filter_map);
        std::vector<FilterManager::StrFilterItemMapT>().swap(attr_filter_map);
        std::vector<FilterManager::NumFilterItemMapT>().swap(num_filter_map);
    }
    LOG(INFO) << "building filter data finished";

    for (size_t i = last_docid + 1; i <= document_manager_->getMaxDocId(); ++i)
    {
        if (i % 100000 == 0)
        {
            LOG(INFO) << "inserted docs: " << i;
        }
        Document doc;
        if (!document_manager_->getDocument(i, doc))
        {
            new_fmi->addDoc(NULL, 0);
            continue;
        }
        Document::property_const_iterator it = doc.findProperty(property_);
        if (it == doc.propertyEnd())
        {
            new_fmi->addDoc(NULL, 0);
            continue;
        }

        izenelib::util::UString text = it->second.get<UString>();
        Algorithm<UString>::to_lower(text);
        text = Algorithm<UString>::trim(text);
        new_fmi->addDoc(text.data(), text.length());
    }
    std::ofstream ofs(orig_text_path_.c_str());
    new_fmi->saveOriginalText(ofs);
    ofs.close();

    LOG(INFO) << "inserted docs: " << document_manager_->getMaxDocId();
    LOG(INFO) << "building fm-index....";

    new_fmi->buildText();
    for (size_t i = 0; i < new_filter_manager->propertyCount(); ++i)
    {
        new_fmi->buildFilter(i);
    }

    {
        WriteLock lock(mutex_);
        fmi_.reset(new_fmi);
        filter_manager_.reset(new_filter_manager);
    }

    ofs.open(fm_index_path_.c_str());
    fmi_->save(ofs);
    filter_manager_->saveFilterId();
    filter_manager_->clearFilterList();
    LOG(INFO) << "building fm-index finished";
}

size_t SuffixMatchManager::longestSuffixMatch(
        const izenelib::util::UString& pattern,
        size_t max_docs,
        std::vector<std::pair<double, uint32_t> >& res_list) const
{
    if (!fmi_) return 0;

    std::vector<std::pair<size_t, size_t> >match_ranges;
    std::vector<uint32_t> docid_list;
    std::vector<size_t> doclen_list;
    size_t max_match;

    {
        ReadLock lock(mutex_);
        if ((max_match = fmi_->longestSuffixMatch(pattern.data(), pattern.length(), match_ranges)) == 0)
            return 0;

        fmi_->getMatchedDocIdList(match_ranges, max_docs, docid_list, doclen_list);
    }

    res_list.resize(docid_list.size());
    for (size_t i = 0; i < res_list.size(); ++i)
    {
        res_list[i].first = double(max_match) / double(doclen_list[i]);
        res_list[i].second = docid_list[i];
    }
    std::sort(res_list.begin(), res_list.end(), std::greater<std::pair<double, uint32_t> >());

    size_t total_match = 0;
    for (size_t i = 0; i < match_ranges.size(); ++i)
    {
        total_match += match_ranges[i].second - match_ranges[i].first;
    }

    return total_match;
}

size_t SuffixMatchManager::AllPossibleSuffixMatch(
        const izenelib::util::UString& pattern_orig,
        size_t max_docs,
        const SearchingMode::SuffixMatchFilterMode& filter_mode,
        const std::vector<QueryFiltering::FilteringType>& filter_param,
        const GroupParam& group_param,
        std::vector<std::pair<double, uint32_t> >& res_list) const
{
    if (!analyzer_) return 0;

    std::vector<std::pair<size_t, size_t> > match_ranges_list;
    std::vector<uint32_t> docid_list;
    std::vector<size_t> doclen_list;
    std::vector<double> max_match_list;

    // tokenize the pattern.
    izenelib::util::UString pattern = pattern_orig;
    Algorithm<UString>::to_lower(pattern);
    string pattern_str;
    pattern.convertString(pattern_str, UString::UTF_8);
    LOG(INFO) << "original query string: " << pattern_str;

    Sentence pattern_sentence(pattern_str.c_str());
    analyzer_->runWithSentence(pattern_sentence);
    std::vector<UString> all_sub_strpatterns;
    LOG(INFO) << "query tokenize by maxprefix match in dictionary: ";
    for (int i = 0; i < pattern_sentence.getCount(0); i++)
    {
        printf("%s, ", pattern_sentence.getLexicon(0, i));
        all_sub_strpatterns.push_back(UString(pattern_sentence.getLexicon(0, i), UString::UTF_8));
    }
    cout << endl;

    size_t match_dic_pattern_num = all_sub_strpatterns.size();

    LOG(INFO) << "query tokenize by maxprefix match in bigram: ";
    for (int i = 0; i < pattern_sentence.getCount(1); i++)
    {
        printf("%s, ", pattern_sentence.getLexicon(1, i));
        all_sub_strpatterns.push_back(UString(pattern_sentence.getLexicon(1, i), UString::UTF_8));
    }
    cout << endl;

    match_ranges_list.reserve(all_sub_strpatterns.size());
    max_match_list.reserve(all_sub_strpatterns.size());

    if (!fmi_) return 0;
    {
        ReadLock lock(mutex_);
        LOG(INFO) << "query tokenize match ranges are: ";
        for (size_t i = 0; i < all_sub_strpatterns.size(); ++i)
        {
            if (all_sub_strpatterns[i].empty())
                continue;
            std::pair<size_t, size_t> sub_match_range;
            size_t matched = fmi_->backwardSearch(all_sub_strpatterns[i].data(),
                    all_sub_strpatterns[i].length(), sub_match_range);
            cout << "match length: " << matched << ", range:" << sub_match_range.first << "," << sub_match_range.second << endl;
            if (matched == all_sub_strpatterns[i].length())
            {
                match_ranges_list.push_back(sub_match_range);
                if (i < match_dic_pattern_num)
                    max_match_list.push_back((double)2.0);
                else
                    max_match_list.push_back((double)1.0);
            }
        }

        if (match_ranges_list.size() == 1)
        {
            fmi_->getMatchedDocIdList(match_ranges_list[0], max_docs, docid_list, doclen_list);
            res_list.resize(docid_list.size());
            for (size_t i = 0; i < res_list.size(); ++i)
            {
                res_list[i].first = double(max_match_list[0]) / double(doclen_list[i]);
                res_list[i].second = docid_list[i];
            }
            std::sort(res_list.begin(), res_list.end(), std::greater<std::pair<double, uint32_t> >());
        }
        else
        {
            std::vector<size_t> prop_id_list;
            std::vector<std::vector<FMIndexType::FilterRangeT> > filter_range_list;
            if (!group_param.isGroupEmpty())
            {
                if (!getAllFilterRangeFromGroupLable_(group_param, prop_id_list, filter_range_list))
                    return 0;
            }
            if (!group_param.isAttrEmpty())
            {
                if (!getAllFilterRangeFromAttrLable_(group_param, prop_id_list, filter_range_list))
                    return 0;
            }
            if (!filter_param.empty())
            {
                if (!getAllFilterRangeFromFilterParam_(filter_param, prop_id_list, filter_range_list))
                    return 0;
            }

            if (filter_mode == SearchingMode::OR_Filter)
            {
                fmi_->getTopKDocIdListByFilter(
                        prop_id_list,
                        filter_range_list,
                        match_ranges_list,
                        max_match_list,
                        max_docs,
                        res_list,
                        doclen_list);
            }
            else if (filter_mode == SearchingMode::AND_Filter)
            {
                // TODO: implement it when fm-index is ready for AND filter.
                assert(false);
            }
            else
            {
                LOG(ERROR) << "unknown filter mode.";
            }
        }
    }

    size_t total_match = 0;
    for (size_t i = 0; i < match_ranges_list.size(); ++i)
    {
        total_match += match_ranges_list[i].second - match_ranges_list[i].first;
    }

    return total_match;
}

bool SuffixMatchManager::getAllFilterRangeFromAttrLable_(
        const GroupParam& group_param,
        std::vector<size_t>& prop_id_list,
        std::vector<std::vector<FMIndexType::FilterRangeT> >& filter_range_list) const
{
    if (filter_manager_ == NULL || attr_property_list_.empty())
    {
        LOG(INFO) << "no filter support.";
        return false;
    }

    FMIndexType::FilterRangeT filter_range;
    FilterManager::FilterIdRange filterid_range;

    size_t prop_id = filter_manager_->getPropertyId(attr_property_list_.front());
    if (prop_id == (size_t)-1) return true;

    std::vector<FMIndexType::FilterRangeT> temp_range_list;
    for (GroupParam::AttrLabelMap::const_iterator cit = group_param.attrLabels_.begin();
            cit != group_param.attrLabels_.end(); ++cit)
    {
        const std::vector<std::string>& attr_values = cit->second;
        for (size_t i = 0; i < attr_values.size(); ++i)
        {
            const UString& attr_filterstr = filter_manager_->FormatAttrPath(UString(cit->first, UString::UTF_8),
                    UString(attr_values[i], UString::UTF_8));
            filterid_range = filter_manager_->getStrFilterIdRange(prop_id, attr_filterstr);
            if (filterid_range.start >= filterid_range.end)
            {
                LOG(WARNING) << "attribute filter id range not found. " << attr_filterstr;
                continue;
            }
            if (!fmi_->getFilterRange(prop_id, std::make_pair(filterid_range.start, filterid_range.end), filter_range))
            {
                LOG(WARNING) << "get filter DocArray range failed.";
                continue;
            }

            LOG(INFO) << "attribute filter DocArray range is : " << filter_range.first << ", " << filter_range.second;
            temp_range_list.push_back(filter_range);
        }
    }

    if (!temp_range_list.empty())
    {
        prop_id_list.push_back(prop_id);
        filter_range_list.push_back(temp_range_list);
    }

    return true;
}

bool SuffixMatchManager::getAllFilterRangeFromGroupLable_(
        const GroupParam& group_param,
        std::vector<size_t>& prop_id_list,
        std::vector<std::vector<FMIndexType::FilterRangeT> >& filter_range_list) const
{
    if (filter_manager_ == NULL)
    {
        LOG(INFO) << "no filter support.";
        return false;
    }

    FMIndexType::FilterRangeT filter_range;
    FilterManager::FilterIdRange filterid_range;

    for (GroupParam::GroupLabelMap::const_iterator cit = group_param.groupLabels_.begin();
            cit != group_param.groupLabels_.end(); ++cit)
    {
        bool is_numeric = numeric_property_list_.find(cit->first) != numeric_property_list_.end();
        bool is_date = std::binary_search(date_property_list_.begin(), date_property_list_.end(), cit->first);
        const GroupParam::GroupPathVec& group_values = cit->second;

        size_t prop_id = filter_manager_->getPropertyId(cit->first);
        if (prop_id == (size_t)-1) continue;

        std::vector<FMIndexType::FilterRangeT> temp_range_list;
        for (size_t i = 0; i < group_values.size(); ++i)
        {
            if (is_numeric)
            {
                bool is_range = false;
                if (!checkLabelParam(*cit, is_range))
                {
                    continue;
                }
                if (is_range)
                {
                    NumericRange range;
                    if (!convertRangeLabel(group_values[i].front(), range))
                        continue;
                    FilterManager::FilterIdRange tmp_range;
                    tmp_range = filter_manager_->getNumFilterIdRangeSmaller(prop_id, std::max(range.first, range.second));
                    filterid_range = filter_manager_->getNumFilterIdRangeLarger(prop_id, std::min(range.first, range.second));
                    filterid_range.start = std::max(filterid_range.start, tmp_range.start);
                    filterid_range.end = std::min(filterid_range.end, tmp_range.end);
                }
                else
                {
                    float value = 0;
                    if (!convertNumericLabel(group_values[i].front(), value))
                        continue;
                    filterid_range = filter_manager_->getNumFilterIdRangeExactly(prop_id, value);
                }
            }
            else if (is_date)
            {
                const std::string& propValue = group_values[i].front();
                faceted::DateStrFormat dsf;
                faceted::DateStrFormat::DateVec date_vec;
                if (!dsf.apiDateStrToDateVec(propValue, date_vec))
                {
                    LOG(WARNING) << "get date from datestr error. " << propValue;
                    continue;
                }
                double value = (double)dsf.createDate(date_vec);
                LOG(INFO) << "date value is : " << value;
                int date_index = date_vec.size() - 1;
                if (date_index < faceted::DateStrFormat::DATE_INDEX_DAY)
                {
                    faceted::DateStrFormat::DateVec end_date_vec = date_vec;
                    // convert to range.
                    if (date_index == faceted::DateStrFormat::DATE_INDEX_MONTH)
                    {
                        end_date_vec.push_back(31);
                    }
                    else if (date_index == faceted::DateStrFormat::DATE_INDEX_YEAR)
                    {
                        end_date_vec.push_back(12);
                        end_date_vec.push_back(31);
                    }
                    else
                    {
                        continue;
                    }
                    double end_value = (double)dsf.createDate(end_date_vec);
                    LOG(INFO) << "end date value is : " << end_value;
                    FilterManager::FilterIdRange tmp_range;
                    tmp_range = filter_manager_->getNumFilterIdRangeSmaller(prop_id, end_value);
                    filterid_range = filter_manager_->getNumFilterIdRangeLarger(prop_id, value);
                    filterid_range.start = std::max(filterid_range.start, tmp_range.start);
                    filterid_range.end = std::min(filterid_range.end, tmp_range.end);
                }
                else
                {
                    filterid_range = filter_manager_->getNumFilterIdRangeExactly(prop_id, value);
                }
            }
            else
            {
                const UString& group_filterstr = filter_manager_->FormatGroupPath(group_values[i]);
                filterid_range = filter_manager_->getStrFilterIdRange(prop_id, group_filterstr);
            }

            if (filterid_range.start >= filterid_range.end)
            {
                LOG(WARNING) << "one of group label filter id range not found.";
                continue;
            }
            if (!fmi_->getFilterRange(prop_id, std::make_pair(filterid_range.start, filterid_range.end), filter_range))
            {
                LOG(WARNING) << "get filter DocArray range failed.";
                continue;
            }

            temp_range_list.push_back(filter_range);
            LOG(INFO) << "group label filter DocArray range is : " << filter_range.first << ", " << filter_range.second;
        }

        if (!temp_range_list.empty())
        {
            prop_id_list.push_back(prop_id);
            filter_range_list.push_back(temp_range_list);
        }
    }

    return true;
}

bool SuffixMatchManager::getAllFilterRangeFromFilterParam_(
        const std::vector<QueryFiltering::FilteringType>& filter_param,
        std::vector<size_t>& prop_id_list,
        std::vector<std::vector<FMIndexType::FilterRangeT> >& filter_range_list) const
{
    if (filter_manager_ == NULL)
    {
        LOG(INFO) << "no filter support.";
        return false;
    }

    FMIndexType::FilterRangeT filter_range;
    FilterManager::FilterIdRange filterid_range;

    for (size_t i = 0; i < filter_param.size(); ++i)
    {
        const QueryFiltering::FilteringType& filtertype = filter_param[i];

        bool is_numeric = numeric_property_list_.find(filtertype.property_) != numeric_property_list_.end()
            || std::binary_search(date_property_list_.begin(), date_property_list_.end(), filtertype.property_);
        size_t prop_id = filter_manager_->getPropertyId(filtertype.property_);
        if (prop_id == (size_t)-1) continue;

        std::vector<FMIndexType::FilterRangeT> temp_range_list;
        for (size_t j = 0; j < filtertype.values_.size(); ++j)
        {
            try
            {
                if (is_numeric)
                {
                    float filter_num = filtertype.values_[j].get<float>();
                    LOG(INFO) << "filter num by : " << filter_num;

                    switch (filtertype.operation_)
                    {
                    case QueryFiltering::LESS_THAN_EQUAL:
                    case QueryFiltering::LESS_THAN:
                        filterid_range = filter_manager_->getNumFilterIdRangeSmaller(prop_id, filter_num);
                        break;

                    case QueryFiltering::GREATER_THAN_EQUAL:
                    case QueryFiltering::GREATER_THAN:
                        filterid_range = filter_manager_->getNumFilterIdRangeLarger(prop_id, filter_num);
                        break;

                    case QueryFiltering::RANGE:
                        {
                            assert(filtertype.values_.size() == 2);
                            if (j >= 1) continue;
                            float filter_num_2 = filtertype.values_[1].get<float>();
                            FilterManager::FilterIdRange tmp_range;
                            tmp_range = filter_manager_->getNumFilterIdRangeSmaller(prop_id, std::max(filter_num, filter_num_2));
                            filterid_range = filter_manager_->getNumFilterIdRangeLarger(prop_id, std::min(filter_num, filter_num_2));
                            filterid_range.start = std::max(filterid_range.start, tmp_range.start);
                            filterid_range.end = std::min(filterid_range.end, tmp_range.end);
                        }
                        break;

                    case QueryFiltering::EQUAL:
                        filterid_range = filter_manager_->getNumFilterIdRangeExactly(prop_id, filter_num);
                        break;

                    default:
                        LOG(WARNING) << "not support filter operation for numeric property in fuzzy searching.";
                        continue;
                    }
                }
                else
                {
                    const std::string& filterstr = filtertype.values_[j].get<std::string>();
                    LOG(INFO) << "filter range by : " << filterstr;

                    switch (filtertype.operation_)
                    {
                    case QueryFiltering::EQUAL:
                    case QueryFiltering::INCLUDE:
                        filterid_range = filter_manager_->getStrFilterIdRange(prop_id, UString(filterstr, UString::UTF_8));
                        break;

                    default:
                        LOG(WARNING) << "not support filter operation for string property in fuzzy searching.";
                        continue;
                    }
                }
            }
            catch (const boost::bad_get &)
            {
                LOG(INFO) << "get filter string failed. boost::bad_get.";
                continue;
            }

            if (filterid_range.start >= filterid_range.end)
            {
                LOG(WARNING) << "filter id range not found. ";
                continue;
            }

            if (!fmi_->getFilterRange(prop_id, std::make_pair(filterid_range.start, filterid_range.end), filter_range))
            {
                LOG(WARNING) << "get filter DocArray range failed.";
                continue;
            }

            temp_range_list.push_back(filter_range);
            LOG(INFO) << "filter range is : " << filterid_range.start << ", " << filterid_range.end;
            LOG(INFO) << "filter DocArray range is : " << filter_range.first << ", " << filter_range.second;
        }

        if (!temp_range_list.empty())
        {
            prop_id_list.push_back(prop_id);
            filter_range_list.push_back(temp_range_list);
        }
    }

    return true;
}

void SuffixMatchManager::buildTokenizeDic()
{
    std::string cma_path;
    LAPool::getInstance()->get_cma_path(cma_path);
    boost::filesystem::path cma_fmindex_dic(cma_path);
    cma_fmindex_dic /= boost::filesystem::path(tokenize_dicpath_);
    LOG(INFO) << "fm-index dictionary path : " << cma_fmindex_dic.c_str() << endl;
    knowledge_ = CMA_Factory::instance()->createKnowledge();
    knowledge_->loadModel( "utf8", cma_fmindex_dic.c_str(), false);
    analyzer_ = CMA_Factory::instance()->createAnalyzer();
    analyzer_->setOption(Analyzer::OPTION_TYPE_POS_TAGGING, 0);
    // using the maxprefix analyzer
    analyzer_->setOption(Analyzer::OPTION_ANALYSIS_TYPE, 100);
    analyzer_->setKnowledge(knowledge_);
    LOG(INFO) << "load dictionary knowledge finished." << endl;

    //izenelib::util::UString testustr("佳能(Canon)test non 单子不存在exist字典(6666) EOS 60D单反套机(EF-S 18-135mm f/3.5-5.6IS）单反镜头单子不存在", UString::UTF_8);
    //Algorithm<UString>::to_lower(testustr);
    //string teststr;
    //testustr.convertString(teststr, UString::UTF_8);
    //Sentence tests(teststr.c_str());
    //analyzer_->runWithSentence(tests);
    //for (int i = 0; i < tests.getCount(0); i++)
    //{
    //    printf("%s, ", tests.getLexicon(0, i));
    //}
    //printf("\n");
    //printf("non dictionary bigram: \n");
    //for (int i = 0; i < tests.getCount(1); i++)
    //{
    //    printf("%s, ", tests.getLexicon(1, i));
    //}
    //printf("\n");
}

}
