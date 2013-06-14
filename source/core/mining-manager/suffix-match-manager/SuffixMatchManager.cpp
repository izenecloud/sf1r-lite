#include "SuffixMatchManager.hpp"
#include "ProductTokenizer.h"
#include <document-manager/DocumentManager.h>
#include <boost/filesystem.hpp>
#include <glog/logging.h>
#include <icma/icma.h>
#include <la-manager/LAPool.h>
#include <common/CMAKnowledgeFactory.h>
#include <mining-manager/util/split_ustr.h>
#include <mining-manager/group-manager/DateStrFormat.h>
#include "FilterManager.h"
#include "FMIndexManager.h"

#include <3rdparty/am/btree/btree_map.h>
#include <util/ustring/algo.hpp>
#include <glog/logging.h>


using namespace cma;
using namespace izenelib::util;

namespace sf1r
{
using namespace faceted;

SuffixMatchManager::SuffixMatchManager(
        const std::string& homePath,
        const std::string& dicpath,
        const std::string& system_resource_path,
        boost::shared_ptr<DocumentManager>& document_manager,
        faceted::GroupManager* groupmanager,
        faceted::AttrManager* attrmanager,
        NumericPropertyTableBuilder* numeric_tablebuilder)
    : data_root_path_(homePath)
    , tokenize_dicpath_(dicpath)
    , system_resource_path_(system_resource_path)
    , document_manager_(document_manager)
    , tokenizer_(NULL)
    , suffixMatchTask_(NULL)
{
    if (!boost::filesystem::exists(homePath))
    {
        boost::filesystem::create_directories(homePath);
    }
    buildTokenizeDic();

    filter_manager_.reset(new FilterManager(document_manager_, groupmanager, data_root_path_,
            attrmanager, numeric_tablebuilder));
    fmi_manager_.reset(new FMIndexManager(data_root_path_, document_manager_, filter_manager_));
}

SuffixMatchManager::~SuffixMatchManager()
{
    if (tokenizer_) delete tokenizer_;
    //if (knowledge_) delete knowledge_;
}

void SuffixMatchManager::setProductMatcher(ProductMatcher* matcher)
{
    if (tokenizer_)
        tokenizer_->SetProductMatcher(matcher);
}

void SuffixMatchManager::addFMIndexProperties(const std::vector<std::string>& property_list, int type, bool finished)
{
    fmi_manager_->addProperties(property_list, (FMIndexManager::PropertyFMType)type);
    if (finished)
    {
        if (!fmi_manager_->loadAll())
        {
            LOG(ERROR) << "load fm index data error!";
        }
    }
}

bool SuffixMatchManager::isStartFromLocalFM() const
{
    return fmi_manager_ && fmi_manager_->isStartFromLocalFM();
}

size_t SuffixMatchManager::longestSuffixMatch(
        const std::string& pattern_orig,
        std::vector<std::string> search_in_properties,
        size_t max_docs,
        std::vector<std::pair<double, uint32_t> >& res_list) const
{
    if (!fmi_manager_ || pattern_orig.empty()) return 0;

    UString pattern(pattern_orig, UString::UTF_8);

    btree::btree_map<uint32_t, double> res_list_map;
    std::vector<uint32_t> docid_list;
    std::vector<size_t> doclen_list;
    size_t max_match;
    size_t total_match = 0;

    {
        ReadLock lock(mutex_);
        for (size_t i = 0; i < search_in_properties.size(); ++i)
        {
            const std::string& property = search_in_properties[i];
            FMIndexManager::RangeListT match_ranges;
            max_match = 0;
            {
                max_match = fmi_manager_->longestSuffixMatch(property, pattern, match_ranges);
                if (max_match == 0)
                    continue;
                LOG(INFO) << "longestSuffixMatch on property: " << property <<
                    ", length: " << max_match;
                for (size_t i = 0; i < match_ranges.size(); ++i)
                {
                    LOG(INFO) << "range " << i << ": " << match_ranges[i].first << "-" << match_ranges[i].second;
                }
                std::vector<double> max_match_list(match_ranges.size(), max_match);
                fmi_manager_->convertMatchRanges(property, max_docs, match_ranges, max_match_list);
                fmi_manager_->getMatchedDocIdList(property, match_ranges, max_docs, docid_list, doclen_list);
            }

            for (size_t j = 0; j < docid_list.size(); ++j)
            {
                assert(doclen_list[j] > 0);
                res_list_map[docid_list[j]] += double(max_match) / double(doclen_list[j]);
            }

            for (size_t j = 0; j < match_ranges.size(); ++j)
            {
                total_match += match_ranges[j].second - match_ranges[j].first;
            }
            docid_list.clear();
            doclen_list.clear();
        }
    }

    res_list.reserve(res_list_map.size());
    for (btree::btree_map<uint32_t, double>::const_iterator cit = res_list_map.begin();
            cit != res_list_map.end(); ++cit)
    {
        res_list.push_back(std::make_pair(cit->second, cit->first));
    }
    std::sort(res_list.begin(), res_list.end(), std::greater<std::pair<double, uint32_t> >());
    if (res_list.size() > max_docs)
        res_list.erase(res_list.begin() + max_docs, res_list.end());
    return total_match;
}

void SuffixMatchManager::GetTokenResults(std::string pattern,
                                std::list<std::pair<UString, double> >& major_tokens,
                                std::list<std::pair<UString, double> >& manor_tokens,
                                UString& analyzedQuery)
{
    tokenizer_->GetTokenResults(pattern, major_tokens, manor_tokens, analyzedQuery);
    //
}

bool SuffixMatchManager::GetSynonymSet_(const UString& pattern, std::vector<UString>& synonym_set, int& setid)
{
    if (!tokenizer_)
    {
        LOG(INFO)<<"tokenizer_ = NULL";
        return false;
    }
    return tokenizer_->GetSynonymSet(pattern, synonym_set, setid);
}

void SuffixMatchManager::ExpandSynonym_(const std::vector<std::pair<UString, double> >& tokens, std::vector<std::vector<std::pair<UString, double> > >& refine_tokens, size_t& major_size)
{
    const size_t tmp_size = major_size;
    std::vector<size_t> set_id;
    for (size_t j = 0; j < tokens.size(); ++j)
    {
        std::vector<UString> synonym_set;
        std::vector<std::pair<UString, double> > tmp_tokens;
        int id = -1, find_flag = -1;
        if (!GetSynonymSet_(tokens[j].first, synonym_set, id))//term doesn't have synonym
        {
            tmp_tokens.push_back(tokens[j]);
            refine_tokens.push_back(tmp_tokens);
            continue;
        }
        for (size_t i = 0; i < set_id.size(); ++i)//check if the synonym set has been added
            if (set_id[i] == id)
            {
                find_flag = i;
                break;
            }
            
        if (-1 == find_flag)//the synonym set hasn't been added
        {
            set_id.push_back(id);
            for (size_t i = 0; i < synonym_set.size(); ++i)
            {
                if (synonym_set[i] == tokens[j].first)
                {
                    tmp_tokens.push_back(std::make_pair(synonym_set[i], tokens[j].second));
                }
                else
                {
                    tmp_tokens.push_back(std::make_pair(synonym_set[i], tokens[j].second * 0.8));                    
                }
            }
            refine_tokens.push_back(tmp_tokens);
        }
        else//has been added
        {
            if (j < tmp_size) --major_size;
            for (size_t i = 0; i < refine_tokens[find_flag].size(); ++i)
                if (refine_tokens[find_flag][i].first == tokens[j].first)
                {
                    refine_tokens[find_flag][i].second = tokens[j].second;
                    break;
                }
        }
    }
    
}

size_t SuffixMatchManager::AllPossibleSuffixMatch(
        bool use_synonym,
        std::list<std::pair<UString, double> > major_tokens,
        std::list<std::pair<UString, double> > minor_tokens,
        std::vector<std::string> search_in_properties,
        size_t max_docs,
        const SearchingMode::SuffixMatchFilterMode& filter_mode,
        const std::vector<QueryFiltering::FilteringType>& filter_param,
        const GroupParam& group_param,
        std::vector<std::pair<double, uint32_t> >& res_list)
{

    btree::btree_map<uint32_t, double> res_list_map;
    std::vector<std::pair<size_t, size_t> > range_list;
    std::vector<std::pair<double, uint32_t> > single_res_list;
    std::vector<double> score_list;
    std::vector<vector<std::pair<UString, double> > > synonym_tokens;
    size_t major_size = 0;    
   
   
    if (use_synonym)
    {
        std::vector<std::pair<UString, double> > tmp_tokens;

        for (std::list<std::pair<UString, double> >::iterator it = major_tokens.begin(); it != major_tokens.end(); ++it, ++major_size)
            tmp_tokens.push_back((*it));
        for (std::list<std::pair<UString, double> >::iterator it = minor_tokens.begin(); it != minor_tokens.end(); ++it)
            tmp_tokens.push_back((*it));       
        ExpandSynonym_(tmp_tokens, synonym_tokens, major_size);
    }

    size_t total_match = 0;

    {
        ReadLock lock(mutex_);
        std::vector<size_t> prop_id_list;
        std::vector<RangeListT> filter_range_list;
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

        std::pair<size_t, size_t> sub_match_range;
        
        for (size_t prop_i = 0; prop_i < search_in_properties.size(); ++prop_i)
        {
            size_t thres = 0;
            const std::string& search_property = search_in_properties[prop_i];
            range_list.reserve(major_tokens.size() + minor_tokens.size());
            score_list.reserve(range_list.size());
            std::vector<std::vector<boost::tuple<size_t, size_t, double> > > synonym_range_list;             
            if (use_synonym)
            {            

                boost::tuple<size_t, size_t, double> tmp_tuple;            
                for (size_t i = 0; i < synonym_tokens.size(); ++i)
                {
                    std::vector<boost::tuple<size_t, size_t, double> > synonym_match_range;
                    for (size_t j = 0; j < synonym_tokens[i].size(); ++j)
                    {    
                        synonym_tokens[i][j].first = Algorithm<UString>::padForAlphaNum(synonym_tokens[i][j].first);
                        if (fmi_manager_->backwardSearch(search_property, synonym_tokens[i][j].first, sub_match_range) == synonym_tokens[i][j].first.length())
                        {
                            tmp_tuple.get<0>() = sub_match_range.first;
                            tmp_tuple.get<1>() = sub_match_range.second;
                            tmp_tuple.get<2>() = synonym_tokens[i][j].second;                        
                            synonym_match_range.push_back(tmp_tuple);
                        }
                    }
                    if (!synonym_match_range.empty())
                    {
                        if (i < major_size) ++thres;
                        synonym_range_list.push_back(synonym_match_range);
                    }
                }

            }
            else
            {
                for (std::list<std::pair<UString, double> >::iterator pit = major_tokens.begin();
                        pit != major_tokens.end(); ++pit)
                {
                    if (pit->first.empty()) continue;
                    Algorithm<UString>::to_lower(pit->first);
                    pit->first = Algorithm<UString>::padForAlphaNum(pit->first);
                    if (fmi_manager_->backwardSearch(search_property, pit->first, sub_match_range) == pit->first.length())
                    {
                        range_list.push_back(sub_match_range);
                        score_list.push_back(pit->second);
                    }
                }

                thres = range_list.size();
    
                for (std::list<std::pair<UString, double> >::iterator pit = minor_tokens.begin();
                        pit != minor_tokens.end(); ++pit)
                {
                    if (pit->first.empty()) continue;
                    Algorithm<UString>::to_lower(pit->first);
                    pit->first = Algorithm<UString>::padForAlphaNum(pit->first);
                    if (fmi_manager_->backwardSearch(search_property, pit->first, sub_match_range) == pit->first.length())
                    {
                        range_list.push_back(sub_match_range);
                        score_list.push_back(pit->second);
                    }
                }
                fmi_manager_->convertMatchRanges(search_property, max_docs, range_list, score_list);
            }
            if (filter_mode == SearchingMode::OR_Filter)
            {
                if (use_synonym)
                {
                    fmi_manager_->getTopKDocIdListByFilter(search_property, prop_id_list, filter_range_list, synonym_range_list, thres,max_docs, single_res_list);                        
                }    
                else
                {
                    fmi_manager_->getTopKDocIdListByFilter(
                            search_property,
                            prop_id_list,
                            filter_range_list,
                            range_list,
                            score_list,
                            thres,
                            max_docs,
                            single_res_list);
                }                             
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
            LOG(INFO) << "topk finished in property : " << search_property;
            size_t oldsize = res_list_map.size();
            for (size_t i = 0; i < single_res_list.size(); ++i)
            {
                res_list_map[single_res_list[i].second] += single_res_list[i].first;
            }
            single_res_list.clear();

            if (use_synonym)  
            {
                for (size_t i = 0; i < synonym_range_list.size(); ++i)
                    for (size_t j = 0; j < synonym_range_list[i].size(); ++j)
                        total_match += synonym_range_list[i][j].get<1>() - synonym_range_list[i][j].get<0>();
            }
            else
            {
                for (size_t i = 0; i < range_list.size(); ++i)
                    total_match += range_list[i].second - range_list[i].first;
            }
            range_list.clear();
            score_list.clear();
            
            LOG(INFO) << "new added docid number: " << res_list_map.size() - oldsize;
        }
    }

    res_list.reserve(res_list_map.size());
    for (btree::btree_map<uint32_t, double>::const_iterator cit = res_list_map.begin();
            cit != res_list_map.end(); ++cit)
    {
        res_list.push_back(std::make_pair(cit->second, cit->first));
    }
    std::sort(res_list.begin(), res_list.end(), std::greater<std::pair<double, uint32_t> >());
    if (res_list.size() > max_docs)
        res_list.erase(res_list.begin() + max_docs, res_list.end());
        
    LOG(INFO) << "all property fuzzy search finished, answer is: "<<res_list.size();

    return total_match;
}

bool SuffixMatchManager::getAllFilterRangeFromAttrLable_(
        const GroupParam& group_param,
        std::vector<size_t>& prop_id_list,
        std::vector<RangeListT>& filter_range_list) const
{
    if (!filter_manager_)
    {
        LOG(INFO) << "no filter support.";
        return false;
    }

    size_t prop_id = filter_manager_->getAttrPropertyId();
    if (prop_id == (size_t)-1) return false;

    FMIndexManager::FilterRangeT filter_range;
    FilterManager::FilterIdRange filterid_range;
    std::vector<FMIndexManager::FilterRangeT> tmp_filter_ranges;
    RangeListT temp_range_list;
    for (GroupParam::AttrLabelMap::const_iterator cit = group_param.attrLabels_.begin();
            cit != group_param.attrLabels_.end(); ++cit)
    {
        const std::vector<std::string>& attr_values = cit->second;
        for (size_t i = 0; i < attr_values.size(); ++i)
        {
            const UString& attr_filterstr = filter_manager_->formatAttrPath(UString(cit->first, UString::UTF_8),
                    UString(attr_values[i], UString::UTF_8));
            filterid_range = filter_manager_->getStrFilterIdRangeExact(prop_id, attr_filterstr);
            if (filterid_range.start >= filterid_range.end)
            {
                LOG(WARNING) << "attribute filter id range not found. " << attr_filterstr;
                continue;
            }
            if (!fmi_manager_->getFilterRange(prop_id, std::make_pair(filterid_range.start, filterid_range.end), filter_range))
            {
                LOG(WARNING) << "get filter DocArray range failed.";
                continue;
            }

            LOG(INFO) << "attribute filter DocArray range is : " << filter_range.first << ", " << filter_range.second;
            temp_range_list.push_back(filter_range);
        }
    }

    if (temp_range_list.empty())
    {
        return false;
    }
    else
    {
        prop_id_list.push_back(prop_id);
        filter_range_list.push_back(temp_range_list);
    }

    return true;
}

bool SuffixMatchManager::getAllFilterRangeFromGroupLable_(
        const GroupParam& group_param,
        std::vector<size_t>& prop_id_list,
        std::vector<RangeListT>& filter_range_list) const
{
    if (!filter_manager_)
    {
        LOG(INFO) << "no filter support.";
        return false;
    }

    FMIndexManager::FilterRangeT filter_range;
    FilterManager::FilterIdRange filterid_range;

    for (GroupParam::GroupLabelMap::const_iterator cit = group_param.groupLabels_.begin();
            cit != group_param.groupLabels_.end(); ++cit)
    {
        bool is_numeric = filter_manager_->isNumericProp(cit->first);
        bool is_date = filter_manager_->isDateProp(cit->first);
        const GroupParam::GroupPathVec& group_values = cit->second;

        size_t prop_id = filter_manager_->getPropertyId(cit->first);
        if (prop_id == (size_t)-1) return false;

        RangeListT temp_range_list;
        for (size_t i = 0; i < group_values.size(); ++i)
        {
            if (group_values[i].empty()) continue;

            if (is_numeric)
            {
                const std::string& str_value = group_values[i][0];
                size_t delimitPos = str_value.find('-');
                if (delimitPos != std::string::npos)
                {
                    int64_t lowerBound = std::numeric_limits<int64_t>::min();
                    int64_t upperBound = std::numeric_limits<int64_t>::max();

                    try
                    {
                        if (delimitPos)
                        {
                            lowerBound = boost::lexical_cast<int64_t>(str_value.substr(0, delimitPos));
                        }

                        if (delimitPos + 1 != str_value.size())
                        {
                            upperBound = boost::lexical_cast<int64_t>(str_value.substr(delimitPos + 1));
                        }
                    }
                    catch (const boost::bad_lexical_cast& e)
                    {
                        LOG(ERROR) << "failed in casting label from " << str_value
                            << " to numeric value, exception: " << e.what();
                        continue;
                    }
                    filterid_range = filter_manager_->getNumFilterIdRangeGreater(prop_id, lowerBound, true);
                    filterid_range.merge(filter_manager_->getNumFilterIdRangeLess(prop_id, upperBound, true));
                }
                else
                {
                    float value = 0;
                    try
                    {
                        value = boost::lexical_cast<float>(str_value);
                    }
                    catch (const boost::bad_lexical_cast& e)
                    {
                        LOG(ERROR) << "failed in casting label from " << str_value
                            << " to numeric value, exception: " << e.what();
                        continue;
                    }
                    filterid_range = filter_manager_->getNumFilterIdRangeExact(prop_id, value);
                }
            }
            else if (is_date)
            {
                const std::string& propValue = group_values[i][0];
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
                    filterid_range = filter_manager_->getNumFilterIdRangeGreater(prop_id, value, true);
                    filterid_range.merge(filter_manager_->getNumFilterIdRangeLess(prop_id, end_value, true));
                }
                else
                {
                    filterid_range = filter_manager_->getNumFilterIdRangeExact(prop_id, value);
                }
            }
            else
            {
                const UString& group_filterstr = filter_manager_->formatGroupPath(group_values[i]);
                filterid_range = filter_manager_->getStrFilterIdRangeExact(prop_id, group_filterstr);
            }

            if (filterid_range.start >= filterid_range.end)
            {
                LOG(WARNING) << "one of group label filter id range not found.";
                continue;
            }
            if (!fmi_manager_->getFilterRange(prop_id, std::make_pair(filterid_range.start, filterid_range.end), filter_range))
            {
                LOG(WARNING) << "get filter DocArray range failed.";
                continue;
            }

            temp_range_list.push_back(filter_range);
            LOG(INFO) << "group label filter DocArray range is : " << filter_range.first << ", " << filter_range.second;
        }

        if (temp_range_list.empty())
        {
            return false;
        }
        else
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
        std::vector<RangeListT>& filter_range_list) const
{
    if (!filter_manager_)
    {
        LOG(INFO) << "no filter support.";
        return false;
    }

    FMIndexManager::FilterRangeT filter_range;
    FilterManager::FilterIdRange filterid_range;

    for (size_t i = 0; i < filter_param.size(); ++i)
    {
        const QueryFiltering::FilteringType& filter = filter_param[i];
        if (filter.values_.empty()) return false;

        bool is_numeric = filter_manager_->isNumericProp(filter.property_)
            || filter_manager_->isDateProp(filter.property_);
        size_t prop_id = filter_manager_->getPropertyId(filter.property_);
        if (prop_id == (size_t)-1) return false;

        RangeListT temp_range_list;
        try
        {
            if (is_numeric)
            {
                double filter_num = filter.values_[0].getNumber();
                LOG(INFO) << "filter num by : " << filter_num;

                switch (filter.operation_)
                {
                    case QueryFiltering::LESS_THAN_EQUAL:
                        filterid_range = filter_manager_->getNumFilterIdRangeLess(prop_id, filter_num, true);
                        break;

                    case QueryFiltering::LESS_THAN:
                        filterid_range = filter_manager_->getNumFilterIdRangeLess(prop_id, filter_num, false);
                        break;

                    case QueryFiltering::GREATER_THAN_EQUAL:
                        filterid_range = filter_manager_->getNumFilterIdRangeGreater(prop_id, filter_num, true);
                        break;

                    case QueryFiltering::GREATER_THAN:
                        filterid_range = filter_manager_->getNumFilterIdRangeGreater(prop_id, filter_num, false);
                        break;

                    case QueryFiltering::RANGE:
                        {
                            if (filter.values_.size() < 2) return false;

                            double filter_num_2 = filter.values_[1].getNumber();
                            filterid_range = filter_manager_->getNumFilterIdRangeGreater(prop_id, std::min(filter_num, filter_num_2), true);
                            filterid_range.merge(filter_manager_->getNumFilterIdRangeLess(prop_id, std::max(filter_num, filter_num_2), true));
                        }
                        break;

                    case QueryFiltering::EQUAL:
                        filterid_range = filter_manager_->getNumFilterIdRangeExact(prop_id, filter_num);
                        break;

                    default:
                        LOG(WARNING) << "not support filter operation for numeric property in fuzzy searching.";
                        return false;
                }
            }
            else
            {
                const std::string& filter_str = filter.values_[0].get<std::string>();
                LOG(INFO) << "filter range by : " << filter_str;

                switch (filter.operation_)
                {
                case QueryFiltering::EQUAL:
                    filterid_range = filter_manager_->getStrFilterIdRangeExact(prop_id, UString(filter_str, UString::UTF_8));
                    break;

                case QueryFiltering::GREATER_THAN:
                    filterid_range = filter_manager_->getStrFilterIdRangeGreater(prop_id, UString(filter_str, UString::UTF_8), false);
                    break;

                case QueryFiltering::GREATER_THAN_EQUAL:
                    filterid_range = filter_manager_->getStrFilterIdRangeGreater(prop_id, UString(filter_str, UString::UTF_8), true);
                    break;

                case QueryFiltering::LESS_THAN:
                    filterid_range = filter_manager_->getStrFilterIdRangeLess(prop_id, UString(filter_str, UString::UTF_8), false);
                    break;

                case QueryFiltering::LESS_THAN_EQUAL:
                    filterid_range = filter_manager_->getStrFilterIdRangeLess(prop_id, UString(filter_str, UString::UTF_8), true);
                    break;

                case QueryFiltering::RANGE:
                    {
                        if (filter.values_.size() < 2) return false;

                        const std::string& filter_str1 = filter.values_[1].get<std::string>();
                        if (filter_str < filter_str1)
                        {
                            filterid_range = filter_manager_->getStrFilterIdRangeGreater(prop_id, UString(filter_str, UString::UTF_8), true);
                            filterid_range.merge(filter_manager_->getStrFilterIdRangeLess(prop_id, UString(filter_str1, UString::UTF_8), true));
                        }
                        else
                        {
                            filterid_range = filter_manager_->getStrFilterIdRangeGreater(prop_id, UString(filter_str1, UString::UTF_8), true);
                            filterid_range.merge(filter_manager_->getStrFilterIdRangeLess(prop_id, UString(filter_str, UString::UTF_8), true));
                        }
                    }
                    break;

                case QueryFiltering::PREFIX:
                    filterid_range = filter_manager_->getStrFilterIdRangePrefix(prop_id, UString(filter_str, UString::UTF_8));
                    break;

                default:
                    LOG(WARNING) << "not support filter operation for string property in fuzzy searching.";
                    return false;
                }
            }
        }
        catch (const boost::bad_get &)
        {
            LOG(INFO) << "get filter string failed. boost::bad_get.";
            return false;
        }

        if (filterid_range.start >= filterid_range.end)
        {
            LOG(WARNING) << "filter id range not found. ";
            return false;
        }

        if (!fmi_manager_->getFilterRange(prop_id, std::make_pair(filterid_range.start, filterid_range.end), filter_range))
        {
            LOG(WARNING) << "get filter DocArray range failed.";
            return false;
        }

        temp_range_list.push_back(filter_range);
        LOG(INFO) << "filter range is : " << filterid_range.start << ", " << filterid_range.end;
        LOG(INFO) << "filter DocArray range is : " << filter_range.first << ", " << filter_range.second;

        if (!temp_range_list.empty())
        {
            prop_id_list.push_back(prop_id);
            filter_range_list.push_back(temp_range_list);
        }
    }
    return true;
}

bool SuffixMatchManager::buildMiningTask()
{
    suffixMatchTask_ = new SuffixMatchMiningTask(
            document_manager_,
            fmi_manager_,
            filter_manager_,
            data_root_path_,
            mutex_);

    if (!suffixMatchTask_)
    {
        LOG(INFO) << "Build SuffixMatch MingTask ERROR";
        return false;
    }
    return false;
}

SuffixMatchMiningTask* SuffixMatchManager::getMiningTask()
{
    if (suffixMatchTask_)
    {
        return suffixMatchTask_;
    }
    return NULL;
}

boost::shared_ptr<FilterManager>& SuffixMatchManager::getFilterManager()
{
    return filter_manager_;
}

void SuffixMatchManager::buildTokenizeDic()
{
    boost::filesystem::path cma_fmindex_dic(system_resource_path_);
    cma_fmindex_dic /= boost::filesystem::path("dict");
    cma_fmindex_dic /= boost::filesystem::path(tokenize_dicpath_);
    LOG(INFO) << "fm-index dictionary path : " << cma_fmindex_dic.c_str() << endl;
    ProductTokenizer::TokenizerType type = tokenize_dicpath_ == "product" ?
        ProductTokenizer::TOKENIZER_DICT : ProductTokenizer::TOKENIZER_CMA;
    tokenizer_ = new ProductTokenizer(type, cma_fmindex_dic.c_str());
}

void SuffixMatchManager::updateFmindex()
{
    LOG (INFO) << "Merge cron-job with fm-index...";
    suffixMatchTask_->preProcess();
    docid_t start_doc = suffixMatchTask_->getLastDocId();
    for (uint32_t docid = start_doc + 1; docid < document_manager_->getMaxDocId(); ++docid)
    {
        Document doc;
        if (document_manager_->getDocument(docid, doc))
        {
            document_manager_->getRTypePropertiesForDocument(docid, doc);
        }
        suffixMatchTask_->buildDocument(docid, doc);
    }
    suffixMatchTask_->postProcess();
    last_doc_id_ = document_manager_->getMaxDocId();
}

}
