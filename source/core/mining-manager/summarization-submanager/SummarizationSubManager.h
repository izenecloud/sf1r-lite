#ifndef SF1R_MINING_MANAGER_MULTI_DOC_SUMMARIZATION_SUBMANAGER_H
#define SF1R_MINING_MANAGER_MULTI_DOC_SUMMARIZATION_SUBMANAGER_H

#include "Summarization.h"

#include <configuration-manager/SummarizeConfig.h>
#include <query-manager/QueryTypeDef.h>

#include <util/ustring/UString.h>
#include <3rdparty/am/stx/btree_map.h>

#include <boost/shared_ptr.hpp>

#include <string>

namespace idmlib
{
namespace util
{
class IDMAnalyzer;
}
}

//#define USE_LOG_SERVER

namespace sf1r
{
class DocumentManager;
class IndexManager;

class ParentKeyStorage;
class SummarizationStorage;
class CommentCacheStorage;
class Corpus;

class MultiDocSummarizationSubManager
{
#ifdef USE_LOG_SERVER
    typedef uint128_t KeyType;
#else
    typedef izenelib::util::UString KeyType;
#endif

public:
    MultiDocSummarizationSubManager(
            const std::string& homePath,
            SummarizeConfig schema,
            boost::shared_ptr<DocumentManager> document_manager,
            boost::shared_ptr<IndexManager> index_manager,
            idmlib::util::IDMAnalyzer* analyzer);

    ~MultiDocSummarizationSubManager();

    void EvaluateSummarization();

    void AppendSearchFilter(
            std::vector<QueryFiltering::FilteringType>& filtingList);

    bool GetSummarizationByRawKey(
            const izenelib::util::UString& rawKey,
            Summarization& result);

private:
    bool DoEvaluateSummarization_(
            Summarization& summarization,
            const KeyType& key,
            const std::vector<izenelib::util::UString>& content_list);

#ifndef USE_LOG_SERVER
    void BuildIndexOfParentKey_();

    void DoInsertBuildIndexOfParentKey_(const std::string& fileName);

    void DoDelBuildIndexOfParentKey_(const std::string& fileName);

    void DoUpdateIndexOfParentKey_(const std::string& fileName);
#endif

    uint32_t GetLastDocid_() const;

    void SetLastDocid_(uint32_t docid) const;

private:
    std::string last_docid_path_;

    SummarizeConfig schema_;
    izenelib::util::UString parent_key_ustr_name_;

    boost::shared_ptr<DocumentManager> document_manager_;
    boost::shared_ptr<IndexManager> index_manager_;

    idmlib::util::IDMAnalyzer* analyzer_;

    CommentCacheStorage* comment_cache_storage_;
    ParentKeyStorage* parent_key_storage_;
    SummarizationStorage* summarization_storage_;

    Corpus* corpus_;
};

}

#endif
