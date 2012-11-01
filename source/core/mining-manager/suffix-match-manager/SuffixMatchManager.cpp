#include "SuffixMatchManager.hpp"
#include <document-manager/DocumentManager.h>
#include <boost/filesystem.hpp>
#include <glog/logging.h>
#include <icma/icma.h>
#include <la-manager/LAPool.h>
#include <mining-manager/util/split_ustr.h>

using namespace cma;

namespace sf1r
{

SuffixMatchManager::SuffixMatchManager(
        const std::string& homePath,
        const std::string& property,
        const std::string& dicpath,
        boost::shared_ptr<DocumentManager>& document_manager)
    : fm_index_path_(homePath + "/" + property + ".fm_idx")
    , property_(property)
    , tokenize_dicpath_(dicpath)
    , document_manager_(document_manager)
    , analyzer_(NULL)
    , knowledge_(NULL)
{
    data_root_path_ = homePath;
    if (!boost::filesystem::exists(homePath))
    {
        boost::filesystem::create_directories(homePath);
    }
    else
    {
        std::ifstream ifs(fm_index_path_.c_str());
        if (ifs)
        {
            fmi_.reset(new FMIndexType(fm_index_path_ + "-filter-data"));
            fmi_->load(ifs);
        }
    }
    buildTokenizeDic();
}

SuffixMatchManager::~SuffixMatchManager()
{
    if(analyzer_) delete analyzer_;
    if(knowledge_) delete knowledge_;
}

void SuffixMatchManager::setGroupFilterProperty(std::vector<std::string>& propertys)
{
    group_property_list_.swap(propertys);
}

void SuffixMatchManager::loadFilterInvertedData(std::vector< FMIndexType::FilterItemT >& filter_inverted_data)
{
    std::ifstream ifs((data_root_path_ + "/filter_inverted.data").c_str());
    if (ifs)
    {
        LOG(INFO) << "loading filter data ";
        size_t filter_num = 0;
        ifs.read((char*)&filter_num, sizeof(filter_num));
        filter_inverted_data.reserve(filter_num);
        LOG(INFO) << "filter total num : " << filter_num;
        for(size_t i = 0; i < filter_num; ++i)
        {
            size_t key_len;
            size_t docid_len;
            ifs.read((char*)&key_len, sizeof(key_len));
            std::string key;
            key.reserve(key_len);
            char tmp[key_len];
            ifs.read(tmp, key_len);
            key.assign(tmp, key_len);
            ifs.read((char*)&docid_len, sizeof(docid_len));
            std::vector<uint32_t> docid_list;
            docid_list.reserve(docid_len);
            LOG(INFO) << "filter num : " << i << ", key=" << key << ", docnum:" << docid_len;
            for(size_t j = 0; j < docid_len; ++j)
            {
                uint32_t did;
                ifs.read((char*)&did, sizeof(did));
                docid_list.push_back(did);
            }
            filter_inverted_data.push_back(std::make_pair(key, docid_list));
        }
    }
}

void SuffixMatchManager::saveFilterInvertedData(const std::vector< FMIndexType::FilterItemT >& filter_inverted_data) const
{
    std::ofstream ofs((data_root_path_ + "/filter_inverted.data").c_str());
    if (ofs)
    {
        LOG(INFO) << "saveing filter data ";
        const size_t filter_num = filter_inverted_data.size();
        ofs.write((const char*)&filter_num, sizeof(filter_num));
        for(size_t i = 0; i < filter_num; ++i)
        {
            const FMIndexType::FilterItemT& item = filter_inverted_data[i];
            std::string keystr;
            item.first.convertString(keystr, UString::UTF_8);
            const size_t key_len = keystr.size();
            const size_t docid_len = item.second.size();
            ofs.write((const char*)&key_len, sizeof(key_len));
            ofs.write(keystr.data(), key_len);
            ofs.write((const char*)&docid_len, sizeof(docid_len));
            LOG(INFO) << "filter num : " << i << ", key=" << keystr << ", docnum:" << docid_len;
            for(size_t j = 0; j < docid_len; ++j)
            {
                const uint32_t& did = item.second[j];
                ofs.write((const char*)&did, sizeof(did));
            }
        }
    }
}

void SuffixMatchManager::buildCollection()
{
    // increase building not support, so delete old filter data.
    if(fmi_)
        fmi_->closedb();
    boost::filesystem::remove_all(fm_index_path_ + "-filter-data");

    std::vector< FMIndexType::FilterItemT > filter_inverted_data;
    FMIndexType* new_fmi = new FMIndexType(fm_index_path_ + "-filter-data");
    size_t last_docid = fmi_ ? fmi_->docCount() : 0;
    if (last_docid)
    {
        std::vector<uint16_t> orig_text;
        std::vector<uint32_t> del_docid_list;
        document_manager_->getDeletedDocIdList(del_docid_list);
        fmi_->reconstructText(del_docid_list, orig_text);
        new_fmi->setOrigText(orig_text);
        loadFilterInvertedData(filter_inverted_data);
    }

    // build the map between the filter string to the index in the inverted array
    std::map<UString, size_t> filter_map;
    for(size_t i = 0; i < filter_inverted_data.size(); ++i)
    {
        filter_map[filter_inverted_data[i].first] = i;
    }

    for (size_t i = last_docid + 1; i <= document_manager_->getMaxDocId(); ++i)
    {
        if (i % 100000 == 0)
        {
            LOG(INFO) << "inserted docs: " << i;
        }
        Document doc;
        document_manager_->getDocument(i, doc);
        for(size_t j = 0; j < group_property_list_.size(); ++j)
        {
            izenelib::util::UString propValue;
            if (document_manager_->getPropertyValue(i, group_property_list_[j], propValue))
            {
                std::string groupstr;
                propValue.convertString(groupstr, UString::UTF_8);
                // split to get each group
                std::vector< vector<izenelib::util::UString> > groupPaths;
                split_group_path(propValue, groupPaths);
                // add docid to each group.
                for(size_t k = 0; k < groupPaths.size(); ++k)
                {
                    for(size_t l = 0; l < groupPaths[k].size(); l++)
                    {
                        std::string strout;
                        groupPaths[k][l].convertString(strout, UString::UTF_8);
                        std::map<UString, size_t>::const_iterator cit = filter_map.find(groupPaths[k][l]);
                        if(cit != filter_map.end())
                        {
                            filter_inverted_data[cit->second].second.push_back(i);
                        }
                        else
                        {
                            // new group 
                            filter_map[groupPaths[k][l]] = filter_inverted_data.size();
                            filter_inverted_data.push_back(std::make_pair(groupPaths[k][l], std::vector<uint32_t>()));
                            filter_inverted_data.back().second.push_back(i);
                        }
                    }
                }
            }
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
    // todo: reorder the vector to make the filter in the same category to put together
    //
    saveFilterInvertedData(filter_inverted_data);
    new_fmi->setAdditionFilterData(filter_inverted_data);

    LOG(INFO) << "inserted docs: " << document_manager_->getMaxDocId();
    LOG(INFO) << "building fm-index";
    new_fmi->build();

    {
        WriteLock lock(mutex_);
        fmi_.reset(new_fmi);
    }

    std::ofstream ofs(fm_index_path_.c_str());
    fmi_->save(ofs);
    LOG(INFO) << "building fm-index finished";
}

size_t SuffixMatchManager::longestSuffixMatch(const izenelib::util::UString& pattern, size_t max_docs, std::vector<uint32_t>& docid_list, std::vector<float>& score_list) const
{
    if (!fmi_) return 0;

    std::vector<std::pair<size_t, size_t> >match_ranges;
    std::vector<size_t> doclen_list;
    size_t max_match;

    {
        ReadLock lock(mutex_);
        if ((max_match = fmi_->longestSuffixMatch(pattern.data(), pattern.length(), match_ranges)) == 0)
            return 0;

        fmi_->getMatchedDocIdList(match_ranges, max_docs, docid_list, doclen_list);
    }

    std::vector<std::pair<float, uint32_t> > res_list(docid_list.size());
    for (size_t i = 0; i < res_list.size(); ++i)
    {
        res_list[i].first = float(max_match) / float(doclen_list[i]);
        res_list[i].second = docid_list[i];
    }
    std::sort(res_list.begin(), res_list.end(), std::greater<std::pair<float, uint32_t> >());

    score_list.resize(doclen_list.size());
    for (size_t i = 0; i < res_list.size(); ++i)
    {
        docid_list[i] = res_list[i].second;
        score_list[i] = res_list[i].first;
    }

    size_t total_match = 0;
    for (size_t i = 0; i < match_ranges.size(); ++i)
    {
        total_match += match_ranges[i].second - match_ranges[i].first;
    }

    return total_match;
}

size_t SuffixMatchManager::AllPossibleSuffixMatch(const izenelib::util::UString& pattern_orig, size_t max_docs,
   std::vector<uint32_t>& docid_list, std::vector<float>& score_list, const izenelib::util::UString& filter_str) const
{
    if(!analyzer_) return 0;

    std::vector<std::pair<size_t, size_t> > match_ranges_list;
    std::vector<size_t> doclen_list;
    std::vector<double> max_match_list;
    std::vector<std::pair<double, uint32_t> > res_list;

    // tokenize the pattern.
    izenelib::util::UString pattern = pattern_orig;
    Algorithm<UString>::to_lower(pattern);
    string pattern_str;
    pattern.convertString(pattern_str, UString::UTF_8);
    LOG(INFO) << " original query string:" << pattern_str;

    Sentence pattern_sentence(pattern_str.c_str());
    analyzer_->runWithSentence(pattern_sentence);
    std::vector< UString > all_sub_strpatterns;
    LOG(INFO) << "query tokenize by maxprefix match in dictionary: ";
    for(int i = 0; i < pattern_sentence.getCount(0); i++)
    {
        printf("%s, ", pattern_sentence.getLexicon(0, i));
        all_sub_strpatterns.push_back(UString(pattern_sentence.getLexicon(0, i), UString::UTF_8));
    }
    printf("\n");

    size_t match_dic_pattern_num = all_sub_strpatterns.size();

    LOG(INFO) << "query tokenize by maxprefix match in bigram: ";
    for(int i = 0; i < pattern_sentence.getCount(1); i++)
    {
        printf("%s, ", pattern_sentence.getLexicon(1, i));
        all_sub_strpatterns.push_back(UString(pattern_sentence.getLexicon(1, i), UString::UTF_8));
    }
    printf("\n");


    if(!fmi_) return 0;
    {
        ReadLock lock(mutex_);
        for(size_t i = 0; i < all_sub_strpatterns.size(); ++i)
        {
            if(all_sub_strpatterns[i].empty())
                continue;
            std::pair<size_t, size_t> sub_match_range;
            size_t matched = fmi_->backwardSearch(all_sub_strpatterns[i].data(),
                    all_sub_strpatterns[i].length(), sub_match_range);
            if(matched == all_sub_strpatterns[i].length())
            {
                match_ranges_list.push_back(sub_match_range);
                if(i < match_dic_pattern_num)
                    max_match_list.push_back((double)2.0);
                else
                    max_match_list.push_back((double)1.0);
            }
        }

        if(match_ranges_list.size() == 1)
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
        else if(!filter_str.empty())
        {
            std::pair<size_t, size_t> filter_range;
            bool ret = fmi_->getFilterRange(filter_str, filter_range);
            if(!ret)
            {
                LOG(INFO) << "filter not found.";
                return 0;
            }
            LOG(INFO) << "filter string range is : " << filter_range.first << ", " << filter_range.second; 
            fmi_->getMatchedTopKDocIdListByFilter(filter_range, match_ranges_list, 
                max_match_list, max_docs, res_list, doclen_list);
        }
        else
        {
            fmi_->getMatchedTopKDocIdList(match_ranges_list,
                    max_match_list, max_docs, res_list, doclen_list);
        }
    }

    score_list.resize(doclen_list.size());
    docid_list.resize(doclen_list.size());
    for (size_t i = 0; i < res_list.size(); ++i)
    {
        docid_list[i] = res_list[i].second;
        score_list[i] = res_list[i].first;
    }

    size_t total_match = 0;
    for (size_t i = 0; i < match_ranges_list.size(); ++i)
    {
        total_match += match_ranges_list[i].second - match_ranges_list[i].first;
    }

    return total_match;
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
    //for(int i = 0; i < tests.getCount(0); i++)
    //{
    //    printf("%s, ", tests.getLexicon(0, i));
    //}
    //printf("\n");
    //printf("non dictionary bigram: \n");
    //for(int i = 0; i < tests.getCount(1); i++)
    //{
    //    printf("%s, ", tests.getLexicon(1, i));
    //}
    //printf("\n");
}

}
