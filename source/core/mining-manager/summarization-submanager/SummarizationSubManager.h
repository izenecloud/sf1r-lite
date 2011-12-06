#ifndef SF1R_MINING_MANAGER_MULTI_DOC_SUMMARIZATION_SUBMANAGER_H
#define SF1R_MINING_MANAGER_MULTI_DOC_SUMMARIZATION_SUBMANAGER_H

#include <configuration-manager/SummarizeConfig.h>


#include <boost/shared_ptr.hpp>

#include <string>

namespace sf1r
{

class DocumentManager;
class IndexManager;

class ParentKeyStorage;

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

private:
    void BuildIndexOfParentKey_();

    void DoInsertBuildIndexOfParentKey_(const std::string& fileName);

    void DoDelBuildIndexOfParentKey_(const std::string& fileName);

    void DoUpdateIndexOfParentKey_(const std::string& fileName);

    SummarizeConfig schema_;
    boost::shared_ptr<DocumentManager> document_manager_;
    boost::shared_ptr<IndexManager> index_manager_;

    ParentKeyStorage* parent_key_storage_;
};

}

#endif
