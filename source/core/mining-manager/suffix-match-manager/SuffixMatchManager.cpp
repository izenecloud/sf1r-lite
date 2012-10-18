#include "SuffixMatchManager.hpp"
#include <document-manager/DocumentManager.h>

#include <boost/filesystem.hpp>
#include <glog/logging.h>

namespace sf1r
{

SuffixMatchManager::SuffixMatchManager(
        const std::string& homePath,
        const std::string& property,
        boost::shared_ptr<DocumentManager>& document_manager)
    : fm_index_path_(homePath + "/" + property + ".fm_idx")
    , property_(property)
    , document_manager_(document_manager)
{
    if (!boost::filesystem::exists(homePath))
    {
        boost::filesystem::create_directories(homePath);
    }
    else
    {
        std::ifstream ifs(fm_index_path_.c_str());
        if (ifs)
        {
            fmi_.reset(new FMIndexType);
            fmi_->load(ifs);
        }
    }
}

SuffixMatchManager::~SuffixMatchManager()
{
}

void SuffixMatchManager::buildCollection()
{
    FMIndexType* new_fmi = new FMIndexType;

    size_t last_docid = fmi_ ? fmi_->docCount() : 0;
    if (last_docid)
    {
        std::vector<uint16_t> orig_text;
        std::vector<uint32_t> del_docid_list;
        document_manager_->getDeletedDocIdList(del_docid_list);
        fmi_->reconstructText(del_docid_list, orig_text);
        new_fmi->setOrigText(orig_text);
    }

    for (size_t i = last_docid + 1; i <= document_manager_->getMaxDocId(); ++i)
    {
        if (i % 100000 == 0)
        {
            LOG(INFO) << "inserted docs: " << i;
        }
        Document doc;
        document_manager_->getDocument(i, doc);
        Document::property_const_iterator it = doc.findProperty(property_);
        if (it == doc.propertyEnd())
        {
            new_fmi->addDoc(NULL, 0);
            continue;
        }

        const izenelib::util::UString& text = it->second.get<UString>();
        new_fmi->addDoc(text.data(), text.length());

    }
    LOG(INFO) << "inserted docs: " << document_manager_->getMaxDocId();

    LOG(INFO) << "building fm-index";
    new_fmi->build();

    {
        WriteLock lock(mutex_);
        fmi_.reset(new_fmi);
    }

    std::ofstream ofs(fm_index_path_.c_str());
    fmi_->save(ofs);
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

}
