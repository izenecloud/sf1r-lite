#include "SummarizationSubManager.h"
#include "ParentKeyStorage.h"
#include "SummarizationStorage.h"
#include "CommentCacheStorage.h"
#include "splm.h"

#include <index-manager/IndexManager.h>
#include <document-manager/DocumentManager.h>
#include <la-manager/LAPool.h>

#include <common/ScdParser.h>
#ifdef USE_LOG_SERVER
#include <common/Utilities.h>
#endif
#include <idmlib/util/idm_analyzer.h>

#include <boost/filesystem.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/algorithm/string.hpp>

#include <glog/logging.h>

#include <iostream>
#include <vector>
#include <algorithm>

using izenelib::util::UString;
using namespace izenelib::ir::indexmanager;
namespace bfs = boost::filesystem;

namespace sf1r
{

static const UString DOCID("DOCID", UString::UTF_8);

bool CheckParentKeyLogFormat(
        const SCDDocPtr& doc,
        const UString& parent_key_name)
{
    if (doc->size() != 2) return false;
    const UString& first = (*doc)[0].first;
    const UString& second = (*doc)[1].first;
    //FIXME case insensitive compare, but it requires extra string conversion,
    //which introduces unnecessary memory fragments
    return (first == DOCID && second == parent_key_name);
}

struct IsParentKeyFilterProperty
{
    const std::string& parent_key_property;

    IsParentKeyFilterProperty(const std::string& property)
        : parent_key_property(property)
    {}

    bool operator()(const QueryFiltering::FilteringType& filterType)
    {
        return boost::iequals(parent_key_property, filterType.property_);
    }
};


MultiDocSummarizationSubManager::MultiDocSummarizationSubManager(
        const std::string& homePath,
        SummarizeConfig schema,
        boost::shared_ptr<DocumentManager> document_manager,
        boost::shared_ptr<IndexManager> index_manager,
        idmlib::util::IDMAnalyzer* analyzer)
    : schema_(schema)
    , parent_key_ustr_name_(schema_.parentKey, UString::UTF_8)
    , document_manager_(document_manager)
    , index_manager_(index_manager)
    , analyzer_(analyzer)
    , comment_cache_storage_(new CommentCacheStorage(homePath))
    , parent_key_storage_(new ParentKeyStorage(homePath, comment_cache_storage_))
    , summarization_storage_(new SummarizationStorage(homePath))
    , corpus_(new Corpus())
{
    if (!schema_.parentKeyLogPath.empty())
        bfs::create_directories(schema_.parentKeyLogPath);
}

MultiDocSummarizationSubManager::~MultiDocSummarizationSubManager()
{
    delete parent_key_storage_;
    delete summarization_storage_;
    delete comment_cache_storage_;
    delete corpus_;
}

void MultiDocSummarizationSubManager::EvaluateSummarization()
{
    BuildIndexOfParentKey_();

    if (schema_.parentKeyLogPath.empty())
    {
        for (uint32_t i = 1; i <= document_manager_->getMaxDocId(); i++)
        {
            Document doc;
            document_manager_->getDocument(i, doc);
            Document::property_const_iterator kit = doc.findProperty(schema_.foreignKeyPropName);
            if (kit == doc.propertyEnd())
                continue;

            Document::property_const_iterator cit = doc.findProperty(schema_.contentPropName);
            if (cit == doc.propertyEnd())
                continue;

            const UString& key = kit->second.get<UString>();
            const UString& content = cit->second.get<UString>();
#ifdef USE_LOG_SERVER
            std::string key_str;
            key.convertString(key_str, UString::UTF_8);
            comment_cache_storage_->AppendUpdate(Utilities::md5ToUint128(key_str), i, content);
#else
            comment_cache_storage_->AppendUpdate(key, i, content);
#endif
        }
    }
    else
    {
        for (uint32_t i = 1; i <= document_manager_->getMaxDocId(); i++)
        {
            Document doc;
            document_manager_->getDocument(i, doc);
            Document::property_const_iterator kit = doc.findProperty(schema_.foreignKeyPropName);
            if (kit == doc.propertyEnd())
                continue;

            Document::property_const_iterator cit = doc.findProperty(schema_.contentPropName);
            if (cit == doc.propertyEnd())
                continue;

            const UString& key = kit->second.get<UString>();
#ifdef USE_LOG_SERVER
            std::string key_str;
            key.convertString(key_str, UString::UTF_8);
            KeyType parent_key;
            if (!parent_key_storage_->GetParent(Utilities::md5ToUint128(key_str), parent_key))
                continue;
#else
            UString parent_key;
            if (!parent_key_storage_->GetParent(key, parent_key))
                continue;
#endif

            const UString& content = cit->second.get<UString>();
            comment_cache_storage_->AppendUpdate(parent_key, i, content);
        }
    }
    comment_cache_storage_->Flush();

    for (int32_t i = 1; i <= comment_cache_storage_->getDirtyKeyCount(); i++)
    {
        KeyType key;
        comment_cache_storage_->GetDirtyKey(i, key);

        CommentCacheStorage::CommentCacheItemType commentCacheItem;
        comment_cache_storage_->GetCommentCache(key, commentCacheItem);

        Summarization summarization(commentCacheItem.first);
        DoEvaluateSummarization_(summarization, key, commentCacheItem.second);
    }
    comment_cache_storage_->ClearDirtyKey();
    summarization_storage_->Flush();
}

void MultiDocSummarizationSubManager::DoEvaluateSummarization_(
        Summarization& summarization,
        const KeyType& key,
        const std::vector<UString>& content_list)
{
    if (!summarization_storage_->IsRebuildSummarizeRequired(key, summarization))
        return;

#define MAXDOC 100

    ilplib::langid::Analyzer* langIdAnalyzer = LAPool::getInstance()->getLangId();

    std::string key_str;
#ifdef USE_LOG_SERVER
    Utilities::uint128ToUuid(key, key_str);
    UString key_ustr(key_str, UString::UTF_8);
    corpus_->start_new_coll(key_ustr);
#else
    key.convertString(key_str, UString::UTF_8);
    corpus_->start_new_coll(key);
#endif
    corpus_->start_new_doc(); // XXX

    for (std::vector<UString>::const_iterator it = content_list.begin();
            it != content_list.end(); ++it)
    {
        // XXX
        // corpus_->start_new_doc();

        const UString& content = *it;
        UString sentence;
        std::size_t startPos = 0;
        while (std::size_t len = langIdAnalyzer->sentenceLength(content, startPos))
        {
            sentence.assign(content, startPos, len);

            corpus_->start_new_sent(sentence);

            std::vector<UString> word_list;
            analyzer_->GetStringList(sentence, word_list);
            for (std::vector<UString>::const_iterator wit = word_list.begin();
                    wit != word_list.end(); ++wit)
            {
                corpus_->add_word(*wit);
            }

            startPos += len;
        }
    }
    corpus_->start_new_sent();
    corpus_->start_new_doc();
    corpus_->start_new_coll();

    std::map<UString, std::vector<std::pair<double, UString> > > summaries;
//#define DEBUG_SUMMARIZATION
#ifdef DEBUG_SUMMARIZATION
    std::cout << "Begin evaluating: " << key_str << std::endl;
#endif
    //if (content_list.size() < 2000 && corpus_->ntotal() < 100000)
    {
        //SPLM::generateSummary(summaries, *corpus_, SPLM::SPLM_RI);
    }
    //else
    {
        SPLM::generateSummary(summaries, *corpus_, SPLM::SPLM_NONE);
    }
#ifdef DEBUG_SUMMARIZATION
    std::cout << "End evaluating: " << key_str << std::endl;
#endif

    //XXX store the generated summary list
#ifdef USE_LOG_SERVER
    std::vector<std::pair<double, UString> >& summary_list = summaries[key_ustr];
#else
    std::vector<std::pair<double, UString> >& summary_list = summaries[key];
#endif
    if (!summary_list.empty())
    {
#ifdef DEBUG_SUMMARIZATION
        for (uint32_t i = 0; i < summary_list.size(); i++)
        {
            std::string sent;
            summary_list[i].second.convertString(sent, UString::UTF_8);
            std::cout << "\t" << sent << std::endl;
        }
#endif
        summarization.insertProperty("overview", summary_list);
    }
    summarization_storage_->Update(key, summarization);

    corpus_->reset();
}

bool MultiDocSummarizationSubManager::GetSummarizationByRawKey(
        const UString& rawKey,
        Summarization& result)
{
#ifdef USE_LOG_SERVER
    std::string key_str;
    rawKey.convertString(key_str, UString::UTF_8);
    return summarization_storage_->Get(Utilities::uuidToUint128(key_str), result);
#else
    return summarization_storage_->Get(rawKey, result);
#endif
}

void MultiDocSummarizationSubManager::AppendSearchFilter(
        std::vector<QueryFiltering::FilteringType>& filtingList)
{
    ///When search filter is based on ParentKey, get its associated values,
    ///and add those values to filter conditions.
    ///The typical situation of this happen when :
    ///SELECT * FROM comments WHERE product_type="foo"
    ///This hook will translate the semantic into:
    ///SELECT * FROM comments WHERE product_id="1" OR product_id="2" ...

    typedef std::vector<QueryFiltering::FilteringType>::iterator IteratorType;
    IteratorType it = std::find_if(filtingList.begin(),
            filtingList.end(), IsParentKeyFilterProperty(schema_.parentKey));
    if (it != filtingList.end())
    {
        const std::vector<PropertyValue>& filterParam = it->values_;
        if (!filterParam.empty())
        {
            try
            {
                const std::string& paramValue = get<std::string>(filterParam[0]);
#ifdef USE_LOG_SERVER
                KeyType param = Utilities::uuidToUint128(paramValue);
#else
                KeyType param(paramValue, UString::UTF_8);
#endif
                std::vector<KeyType> results;
                if (parent_key_storage_->GetChildren(param, results))
                {
                    BTreeIndexerManager* pBTreeIndexer = index_manager_->getBTreeIndexer();
                    QueryFiltering::FilteringType filterRule;
                    filterRule.operation_ = QueryFiltering::INCLUDE;
                    filterRule.property_ = schema_.foreignKeyPropName;
                    std::vector<KeyType>::const_iterator rit = results.begin();
                    for (; rit != results.end(); ++rit)
                    {
#ifdef USE_LOG_SERVER
                        UString result(Utilities::uint128ToMD5(*rit), UString::UTF_8);
#else
                        const KeyType& result = *rit;
#endif
                        if (pBTreeIndexer->seek(schema_.foreignKeyPropName, result))
                        {
                            ///Protection
                            ///Or else, too many unexisted keys are added
                            PropertyValue v(result);
                            filterRule.values_.push_back(v);
                        }
                    }
                    //filterRule.logic_ = QueryFiltering::OR;
                    filtingList.erase(it);
                    //it->logic_ = QueryFiltering::OR;
                    filtingList.push_back(filterRule);
                }
            }
            catch (const boost::bad_get &)
            {
                filtingList.erase(it);
                return;
            }
        }
    }
}

void MultiDocSummarizationSubManager::BuildIndexOfParentKey_()
{
    if (schema_.parentKeyLogPath.empty()) return;
    ScdParser parser(UString::UTF_8);
    std::vector<std::string> scdList;
    static const bfs::directory_iterator kItrEnd;
    for (bfs::directory_iterator itr(schema_.parentKeyLogPath); itr != kItrEnd; ++itr)
    {
        if (bfs::is_regular_file(itr->status()))
        {
            std::string fileName = itr->path().filename().string();
            if (parser.checkSCDFormat(fileName))
            {
                scdList.push_back(itr->path().string());
            }
            else
            {
                LOG(WARNING) << "SCD File not valid " << fileName;
            }
        }
    }

    std::vector<std::string>::const_iterator scd_it = scdList.begin();

    for (; scd_it != scdList.end(); ++scd_it)
    {
        size_t pos = scd_it->rfind("/") + 1;
        string filename = scd_it->substr(pos);

        LOG(INFO) << "Processing SCD file. " << bfs::path(*scd_it).stem();

        switch (parser.checkSCDType(*scd_it))
        {
        case INSERT_SCD:
            DoInsertBuildIndexOfParentKey_(*scd_it);
            LOG(INFO) << "Indexing Finished";
            break;
        case DELETE_SCD:
            DoDelBuildIndexOfParentKey_(*scd_it);
            LOG(INFO) << "Delete Finished";
            break;
        case UPDATE_SCD:
            DoUpdateIndexOfParentKey_(*scd_it);
            LOG(INFO) << "Update Finished";
            break;
        default:
            break;
        }
        parser.load(*scd_it);
    }
    parent_key_storage_->Flush();

    bfs::path bkDir = bfs::path(schema_.parentKeyLogPath) / "backup";
    bfs::create_directory(bkDir);
    LOG(INFO) << "moving " << scdList.size() << " SCD files to directory " << bkDir;

    for (scd_it = scdList.begin(); scd_it != scdList.end(); ++scd_it)
    {
        try
        {
            bfs::rename(*scd_it, bkDir / bfs::path(*scd_it).filename());
        }
        catch (bfs::filesystem_error& e)
        {
            LOG(WARNING) << "exception in rename file " << *scd_it << ": " << e.what();
        }
    }
}

void MultiDocSummarizationSubManager::DoInsertBuildIndexOfParentKey_(
        const std::string& fileName)
{
    ScdParser parser(UString::UTF_8);
    if (!parser.load(fileName)) return;
    for (ScdParser::iterator doc_iter = parser.begin();
            doc_iter != parser.end(); ++doc_iter)
    {
        if (*doc_iter == NULL)
        {
            LOG(WARNING) << "SCD File not valid.";
            return;
        }
        SCDDocPtr doc = (*doc_iter);
        if (!CheckParentKeyLogFormat(doc, parent_key_ustr_name_))
            continue;
#ifdef USE_LOG_SERVER
        std::string parent, child;
        (*doc)[1].second.convertString(parent, UString::UTF_8);
        (*doc)[0].second.convertString(child, UString::UTF_8);
        parent_key_storage_->Insert(Utilities::uuidToUint128(parent), Utilities::md5ToUint128(child));
#else
        parent_key_storage_->Insert((*doc)[1].second, (*doc)[0].second);
#endif
    }
}

void MultiDocSummarizationSubManager::DoUpdateIndexOfParentKey_(
        const std::string& fileName)
{
    ScdParser parser(UString::UTF_8);
    if (!parser.load(fileName)) return;
    for (ScdParser::iterator doc_iter = parser.begin();
            doc_iter != parser.end(); ++doc_iter)
    {
        if (*doc_iter == NULL)
        {
            LOG(WARNING) << "SCD File not valid.";
            return;
        }
        SCDDocPtr doc = (*doc_iter);
        if (!CheckParentKeyLogFormat(doc, parent_key_ustr_name_))
            continue;
#ifdef USE_LOG_SERVER
        std::string parent, child;
        (*doc)[1].second.convertString(parent, UString::UTF_8);
        (*doc)[0].second.convertString(child, UString::UTF_8);
        parent_key_storage_->Update(Utilities::uuidToUint128(parent), Utilities::md5ToUint128(child));
#else
        parent_key_storage_->Update((*doc)[1].second, (*doc)[0].second);
#endif
    }
}

void MultiDocSummarizationSubManager::DoDelBuildIndexOfParentKey_(
        const std::string& fileName)
{
    ScdParser parser(UString::UTF_8);
    if (!parser.load(fileName)) return;
#ifdef USE_LOG_SERVER
    static const KeyType no_parent = 0;
#else
    static const KeyType no_parent;
#endif
    for (ScdParser::iterator doc_iter = parser.begin();
            doc_iter != parser.end(); ++doc_iter)
    {
        if (*doc_iter == NULL)
        {
            LOG(WARNING) << "SCD File not valid.";
            return;
        }
        SCDDocPtr doc = (*doc_iter);
        switch (doc->size())
        {
        case 1:
#ifdef USE_LOG_SERVER
            {
                std::string child;
                (*doc)[0].second.convertString(child, UString::UTF_8);
                parent_key_storage_->Delete(no_parent, Utilities::md5ToUint128(child));
            }
#else
            parent_key_storage_->Delete(no_parent, (*doc)[0].second);
#endif
            break;
        case 2:
#ifdef USE_LOG_SERVER
            {
                std::string parent, child;
                (*doc)[1].second.convertString(parent, UString::UTF_8);
                (*doc)[0].second.convertString(child, UString::UTF_8);
                parent_key_storage_->Delete(Utilities::uuidToUint128(parent), Utilities::md5ToUint128(child));
            }
#else
            parent_key_storage_->Delete((*doc)[1].second, (*doc)[0].second);
#endif
            break;
        default:
            break;
        }
    }
}

}
