#ifndef SF1R_MINING_MANAGER_MULTI_DOC_SUMMARIZATION_SUBMANAGER_H
#define SF1R_MINING_MANAGER_MULTI_DOC_SUMMARIZATION_SUBMANAGER_H

#include <configuration-manager/SummarizeConfig.h>

#include <boost/shared_ptr.hpp>

#include <string>

namespace sf1r
{

class DocumentManager;
class IndexManager;

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

    void ComputeSummarization();

private:
    SummarizeConfig schema_;
    boost::shared_ptr<DocumentManager> document_manager_;
    boost::shared_ptr<IndexManager> index_manager_;

};

}

#endif

