#include "SummarizationSubManager.h"
#include "SummarizationStorage.h"
#include "CommentCacheStorage.h"
#include "splm.h"

#include <index-manager/IndexManager.h>
#include <document-manager/DocumentManager.h>
#include <la-manager/LAPool.h>

#include <log-manager/LogServerRequest.h>
#include <log-manager/LogServerConnection.h>

#include <common/ScdParser.h>
#include <common/Utilities.h>
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
    : last_docid_path_(homePath + "/last_docid.txt")
    , schema_(schema)
    , document_manager_(document_manager)
    , index_manager_(index_manager)
    , analyzer_(analyzer)
    , comment_cache_storage_(new CommentCacheStorage(homePath))
    , summarization_storage_(new SummarizationStorage(homePath))
    , corpus_(new Corpus())
{
}

MultiDocSummarizationSubManager::~MultiDocSummarizationSubManager()
{
    delete summarization_storage_;
    delete comment_cache_storage_;
    delete corpus_;
}

void MultiDocSummarizationSubManager::EvaluateSummarization()
{
    for (uint32_t i = GetLastDocid_() + 1, count = 0; i <= document_manager_->getMaxDocId(); i++)
    {
        Document doc;
        document_manager_->getDocument(i, doc);
        Document::property_const_iterator kit = doc.findProperty(schema_.uuidPropName);
        if (kit == doc.propertyEnd())
            continue;

        Document::property_const_iterator cit = doc.findProperty(schema_.contentPropName);
        if (cit == doc.propertyEnd())
            continue;

        const UString& key = kit->second.get<UString>();
        std::string key_str;
        key.convertString(key_str, UString::UTF_8);

        const UString& content = cit->second.get<UString>();
        comment_cache_storage_->AppendUpdate(Utilities::md5ToUint128(key_str), i, content);

        if (++count % 100000 == 0)
        {
            LOG(INFO) << "Caching comments: " << count;
        }
    }

    std::vector<docid_t> del_docid_list;
    document_manager_->getDeletedDocIdList(del_docid_list);
    for(unsigned int i = 0; i < del_docid_list.size();++i)
    {
        Document doc;
        document_manager_->getDocument(i, doc);
        Document::property_const_iterator kit = doc.findProperty(schema_.uuidPropName);
        if (kit == doc.propertyEnd())
            continue;

        const UString& key = kit->second.get<UString>();
        std::string key_str;
        key.convertString(key_str, UString::UTF_8);
    
        comment_cache_storage_->ExpelUpdate(Utilities::md5ToUint128(key_str), i);
    }

    SetLastDocid_(document_manager_->getMaxDocId());
    comment_cache_storage_->Flush(true);

    {
        CommentCacheStorage::DirtyKeyIteratorType dirtyKeyIt(comment_cache_storage_->dirty_key_db_);
        CommentCacheStorage::DirtyKeyIteratorType dirtyKeyEnd;
        for (uint32_t count = 0; dirtyKeyIt != dirtyKeyEnd; ++dirtyKeyIt)
        {
            const KeyType& key = dirtyKeyIt->first;

            CommentCacheStorage::CommentCacheItemType commentCacheItem;
            comment_cache_storage_->Get(key, commentCacheItem);
            if (commentCacheItem.empty())
            {
                summarization_storage_->Delete(key);
                continue;
            }

            Summarization summarization(commentCacheItem);
            if (DoEvaluateSummarization_(summarization, key, commentCacheItem))
            {
                if (++count % 10000 == 0)
                {
                    LOG(INFO) << "Evaluating summarization: " << count;
                }
            }
        }
        summarization_storage_->Flush();
    } //Destroy dirtyKeyIterator before clearing dirtyKeyDB

    comment_cache_storage_->ClearDirtyKey();
    LOG(INFO) << "Finish evaluating summarization.";
}

bool MultiDocSummarizationSubManager::DoEvaluateSummarization_(
        Summarization& summarization,
        const KeyType& key,
        const std::map<uint32_t, UString>& content_map)
{
    if (!summarization_storage_->IsRebuildSummarizeRequired(key, summarization))
        return false;

#define MAX_SENT_COUNT 1000

    ilplib::langid::Analyzer* langIdAnalyzer = LAPool::getInstance()->getLangId();

    std::string key_str;
    key_str = Utilities::uint128ToUuid(key);
    UString key_ustr(key_str, UString::UTF_8);
    corpus_->start_new_coll(key_ustr);
    corpus_->start_new_doc(); // XXX

    for (std::map<uint32_t, UString>::const_iterator it = content_map.begin();
            it != content_map.end(); ++it)
    {
        // XXX
        // corpus_->start_new_doc();

        const UString& content = it->second;
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

        // Limit the max count of sentences to be summarized
        if (corpus_->nsents() >= MAX_SENT_COUNT)
            break;
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
    std::vector<std::pair<double, UString> >& summary_list = summaries[key_ustr];
    bool ret = !summary_list.empty();

    if (ret)
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
        summarization_storage_->Update(key, summarization);
    }
    corpus_->reset();

    return ret;
}

bool MultiDocSummarizationSubManager::GetSummarizationByRawKey(
        const UString& rawKey,
        Summarization& result)
{
    std::string key_str;
    rawKey.convertString(key_str, UString::UTF_8);
    return summarization_storage_->Get(Utilities::uuidToUint128(key_str), result);
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
            filtingList.end(), IsParentKeyFilterProperty(schema_.uuidPropName));
    if (it != filtingList.end())
    {
        const std::vector<PropertyValue>& filterParam = it->values_;
        if (!filterParam.empty())
        {
            try
            {
                const std::string& paramValue = get<std::string>(filterParam[0]);
                KeyType param = Utilities::uuidToUint128(paramValue);

                LogServerConnection& conn = LogServerConnection::instance();
                GetDocidListRequest req;
                UUID2DocidList resp;

                req.param_.uuid_ = param;
                conn.syncRequest(req, resp);
                if (req.param_.uuid_ != resp.uuid_) return;

                BTreeIndexerManager* pBTreeIndexer = index_manager_->getBTreeIndexer();
                QueryFiltering::FilteringType filterRule;
                filterRule.operation_ = QueryFiltering::INCLUDE;
                filterRule.property_ = schema_.docidPropName;
                for (std::vector<KeyType>::const_iterator rit = resp.docidList_.begin();
                        rit != resp.docidList_.end(); ++rit)
                {
                    UString result(Utilities::uint128ToMD5(*rit), UString::UTF_8);
                    if (pBTreeIndexer->seek(schema_.docidPropName, result))
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
            catch (const boost::bad_get &)
            {
                filtingList.erase(it);
                return;
            }
        }
    }
}

uint32_t MultiDocSummarizationSubManager::GetLastDocid_() const
{
    std::ifstream ifs(last_docid_path_.c_str());

    if (!ifs) return 0;

    uint32_t docid;
    ifs >> docid;
    return docid;
}

void MultiDocSummarizationSubManager::SetLastDocid_(uint32_t docid) const
{
    std::ofstream ofs(last_docid_path_.c_str());

    if (ofs) ofs << docid;
}

}
