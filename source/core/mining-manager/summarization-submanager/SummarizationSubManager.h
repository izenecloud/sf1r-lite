#ifndef SF1R_MINING_MANAGER_MULTI_DOC_SUMMARIZATION_SUBMANAGER_H
#define SF1R_MINING_MANAGER_MULTI_DOC_SUMMARIZATION_SUBMANAGER_H

#include "Summarization.h"

#include <configuration-manager/SummarizeConfig.h>
#include <query-manager/QueryTypeDef.h>

#include <util/ustring/UString.h>

#include <boost/shared_ptr.hpp>

#include <string>

namespace sf1r
{
using izenelib::util::UString;

class DocumentManager;
class IndexManager;

class ParentKeyStorage;
class SummarizationStorage;
class Corpus;

class MultiDocSummarizationSubManager
{
public:
    MultiDocSummarizationSubManager(
            const std::string& homePath,
            SummarizeConfig schema,
            boost::shared_ptr<DocumentManager> document_manager,
            boost::shared_ptr<IndexManager> index_manager
    );

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
            const std::vector<uint32_t>& docs);

    void BuildIndexOfParentKey_();

    void DoInsertBuildIndexOfParentKey_(const std::string& fileName);

    void DoDelBuildIndexOfParentKey_(const std::string& fileName);

    void DoUpdateIndexOfParentKey_(const std::string& fileName);

    SummarizeConfig schema_;
    boost::shared_ptr<DocumentManager> document_manager_;
    boost::shared_ptr<IndexManager> index_manager_;

    ParentKeyStorage* parent_key_storage_;
    SummarizationStorage* summarization_storage_;

    UString parent_key_ustr_name_;
    Corpus* corpus_;
};

}

#endif
