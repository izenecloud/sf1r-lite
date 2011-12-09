#ifndef SF1R_MINING_MANAGER_MULTI_DOC_SUMMARIZATION_SUBMANAGER_H
#define SF1R_MINING_MANAGER_MULTI_DOC_SUMMARIZATION_SUBMANAGER_H

#include "Summarization.h"

#include <configuration-manager/SummarizeConfig.h>
#include <query-manager/QueryTypeDef.h>

#include <util/ustring/UString.h>
#include <3rdparty/am/stx/btree_map.h>

#include <boost/shared_ptr.hpp>
#include <boost/unordered_map.hpp>

#include <string>

namespace idmlib
{
namespace util
{
class IDMAnalyzer;
}
}

namespace sf1r
{
using izenelib::util::UString;

class DocumentManager;
class IndexManager;

class ParentKeyStorage;
class SummarizationStorage;
class CommentCacheStorage;
class Corpus;

class MultiDocSummarizationSubManager
{
    typedef boost::unordered_map<uint32_t, UString> CommentCacheItemType;

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
            const UString& rawKey,
            Summarization& result);

private:
    void DoEvaluateSummarization_(
            const UString& key,
            const CommentCacheItemType& comment_cache_item);

    void BuildIndexOfParentKey_();

    void DoInsertBuildIndexOfParentKey_(const std::string& fileName);

    void DoDelBuildIndexOfParentKey_(const std::string& fileName);

    void DoUpdateIndexOfParentKey_(const std::string& fileName);

private:
    SummarizeConfig schema_;
    UString parent_key_ustr_name_;

    boost::shared_ptr<DocumentManager> document_manager_;
    boost::shared_ptr<IndexManager> index_manager_;

    idmlib::util::IDMAnalyzer* analyzer_;

    ParentKeyStorage* parent_key_storage_;
    SummarizationStorage* summarization_storage_;
    CommentCacheStorage* comment_cache_storage_;

    Corpus* corpus_;
};

}

#endif
